#pragma once
#include <Arduino.h>

class PowerManager {
public:
    void begin(unsigned long sleepTimeoutMs);
    void notifyUserActivity();
    void update();

    bool shouldSleep() const;
    float readBatteryVoltage() const;
    void enterDeepSleep();

private:
    unsigned long _sleepTimeoutMs = 30000;
    unsigned long _lastActivityMs = 0;
    bool _shouldSleep = false;
};