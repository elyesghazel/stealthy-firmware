#pragma once
#include <Arduino.h>
#include <vector>
#include <esp_wifi_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

struct ProbeEntry {
    char     ssid[33];   // max 32 + null terminator
    uint8_t  mac[6];     // first-seen MAC
    int8_t   rssi;       // most recent RSSI
    uint32_t lastSeen;   // millis()
    uint16_t count;      // total probe count
};

class WifiKarma {
public:
    WifiKarma();

    // Probe sniffing — passive, listen for 802.11 probe requests
    void startSniff();
    void stopSniff();
    bool isSniffing() const { return _sniffing; }

    // Karma — clone a target SSID as an open AP
    bool startKarma(const String& ssid);
    void stopKarma();
    bool isKarmaRunning()   const { return _karmaRunning; }
    const String& getKarmaSsid() const { return _karmaSsid; }

    // Probe data (thread-safe copy, sorted by count descending)
    std::vector<ProbeEntry> getProbes() const;
    int  getProbeCount() const;
    void clearProbes();

    static constexpr int MAX_PROBES = 64;

private:
    static void promiscCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static WifiKarma* _instance;

    // Fixed-size array avoids heap allocation inside the callback
    ProbeEntry _probes[MAX_PROBES];
    int        _probeCount = 0;
    mutable portMUX_TYPE _mux;

    // add debug
    void _debugPrintProbes() const;

    bool   _sniffing     = false;
    bool   _karmaRunning = false;
    String _karmaSsid;
};
