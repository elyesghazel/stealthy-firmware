#pragma once
#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct ApInfo {
    String ssid;
    uint8_t bssid[6];
    int32_t rssi;
    int32_t channel;
};

class WifiDeauth {
public:
    bool scan();
    bool startDeauth(int apIndex);
    void stop();
    bool isRunning() const;

    const std::vector<ApInfo>& getApList() const;
    const String& getTargetSsid() const;

    static const char* rssiLabel(int32_t rssi);

private:
    static void deauthTask(void* pvParam);
    void runLoop();
    void sendDeauthFrame(const uint8_t bssid[6], uint8_t channel);

    std::vector<ApInfo> _apList;
    int _targetIndex = -1;
    String _targetSsid;

    TaskHandle_t _taskHandle = nullptr;
    volatile bool _running = false;
};
