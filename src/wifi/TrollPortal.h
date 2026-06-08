#pragma once
#include <WebServer.h>
#include <DNSServer.h>
#include <Arduino.h>
#include <vector>
#include <esp_wifi_types.h>
#include <freertos/portmacro.h>

class TrollPortal {
public:
    // Call before begin() — set which SSIDs to intercept probe requests for
    void setSSIDs(const std::vector<String>& ssids);

    // apSsid: initial/fallback AP SSID before any karma match
    bool begin(const String& apSsid);
    void stop();
    // Must be called from main loop — handles SSID switch + probe response injection
    void update();
    bool isRunning() const;

private:
    static void probeCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void injectProbeResponse(const uint8_t* dest, const char* ssid, uint8_t ssidLen);
    void changeApSsid(const char* ssid, uint8_t ssidLen);
    void setupRoutes();

    static TrollPortal* _instance;

    DNSServer _dns;
    WebServer _server;
    bool      _running = false;

    // ISR-safe fixed SSID list (no std::vector/String in callback)
    static constexpr int MAX_SSIDS = 20;
    char _ssids[MAX_SSIDS][33];
    int  _ssidCount = 0;

    uint8_t _apBssid[6];

    // Pending karma match (written from WiFi task callback, read in update())
    volatile bool _pendingProbe = false;
    char     _pendingSsid[33];
    uint8_t  _pendingDest[6];
    uint8_t  _pendingLen  = 0;
    portMUX_TYPE _mux;
};
