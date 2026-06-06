#pragma once
#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class WifiSpammer {
public:
    void setSSIDs(const std::vector<String>& ssids);
    bool begin();
    void stop();
    bool isRunning() const;
    int ssidCount() const;

private:
    static void spamTask(void* pvParam);
    void runLoop();
    void sendBeacon(const char* ssid, uint8_t index, uint8_t channel);
    void buildBssid(uint8_t index, uint8_t out[6]);

    std::vector<String> _ssids;
    TaskHandle_t _taskHandle = nullptr;
    volatile bool _running = false;
};
