#include "WifiSpammer.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_timer.h>

void WifiSpammer::buildBssid(uint8_t index, uint8_t out[6]) {
    out[0] = 0x02; out[1] = 0x42; out[2] = 0xAC;
    out[3] = 0x11; out[4] = 0x00; out[5] = index;
}

void WifiSpammer::setSSIDs(const std::vector<String>& ssids) {
    _ssids = ssids;
    if (_ssids.size() > 20) {
        _ssids.resize(20);
    }
}

bool WifiSpammer::begin() {
    if (_running) return true;
    if (_ssids.empty()) {
        Serial.println("[WifiSpammer] no SSIDs configured");
        return false;
    }

    // Ensure STA is available without killing a running AP (portal)
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL)    WiFi.mode(WIFI_STA);
    else if (mode == WIFI_MODE_AP) WiFi.mode(WIFI_AP_STA);

    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

    _running = true;
    BaseType_t ok = xTaskCreatePinnedToCore(
        spamTask, "wifi_spam", 4096, this, 1, &_taskHandle, 0
    );

    if (ok != pdPASS) {
        _running = false;
        return false;
    }

    Serial.printf("[WifiSpammer] started, %d SSIDs\n", (int)_ssids.size());
    return true;
}

void WifiSpammer::stop() {
    if (!_running) return;
    _running = false;
    delay(150);
    _taskHandle = nullptr;
    WiFi.mode(WIFI_OFF);
    Serial.println("[WifiSpammer] stopped");
}

bool WifiSpammer::isRunning() const {
    return _running;
}

int WifiSpammer::ssidCount() const {
    return (int)_ssids.size();
}

void WifiSpammer::spamTask(void* pvParam) {
    static_cast<WifiSpammer*>(pvParam)->runLoop();
    vTaskDelete(nullptr);
}

void WifiSpammer::runLoop() {
    while (_running) {
        for (size_t i = 0; i < _ssids.size() && _running; i++) {
            sendBeacon(_ssids[i].c_str(), (uint8_t)(i & 0xFF), 6);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void WifiSpammer::sendBeacon(const char* ssid, uint8_t index, uint8_t channel) {
    uint8_t bssid[6];
    buildBssid(index, bssid);

    uint8_t ssidLen = (uint8_t)strnlen(ssid, 32);

    // frame: 24B header + 12B fixed params + 2+ssidLen SSID tag + 10B rates + 3B DS param
    uint8_t frame[100];
    memset(frame, 0, sizeof(frame));
    int i = 0;

    // 802.11 Management Header
    frame[i++] = 0x80; frame[i++] = 0x00; // Frame Control: beacon
    frame[i++] = 0x00; frame[i++] = 0x00; // Duration
    for (int j = 0; j < 6; j++) frame[i++] = 0xFF; // DA: broadcast
    memcpy(frame + i, bssid, 6); i += 6;             // SA
    memcpy(frame + i, bssid, 6); i += 6;             // BSSID
    frame[i++] = 0x00; frame[i++] = 0x00;            // Sequence

    // Fixed Parameters
    uint64_t ts = (uint64_t)esp_timer_get_time();
    memcpy(frame + i, &ts, 8); i += 8;               // Timestamp
    frame[i++] = 0x64; frame[i++] = 0x00;            // Beacon interval: 100 TU
    frame[i++] = 0x21; frame[i++] = 0x04;            // Capability: ESS + short slot

    // Tagged Parameters
    frame[i++] = 0x00; frame[i++] = ssidLen;         // Tag: SSID
    memcpy(frame + i, ssid, ssidLen); i += ssidLen;

    frame[i++] = 0x01; frame[i++] = 0x08;            // Tag: Supported Rates
    frame[i++] = 0x82; frame[i++] = 0x84; frame[i++] = 0x8b; frame[i++] = 0x96;
    frame[i++] = 0x0c; frame[i++] = 0x12; frame[i++] = 0x18; frame[i++] = 0x24;

    frame[i++] = 0x03; frame[i++] = 0x01; frame[i++] = channel; // Tag: DS Param

    esp_wifi_80211_tx(WIFI_IF_STA, frame, i, false);
}
