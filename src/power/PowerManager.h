#pragma once
#include <Arduino.h>

class PowerManager {
public:
    void begin(unsigned long sleepTimeoutMs);
    void notifyUserActivity();
    void update();

    bool shouldSleep() const;
    float readBatteryVoltage() const;
    int batteryPercent() const;
    bool isUsbPowered() const;
    void enterDeepSleep();

private:
    unsigned long _sleepTimeoutMs = 30000;
    unsigned long _lastActivityMs = 0;
    bool _shouldSleep = false;

    mutable float _cachedBatteryVoltage = 3.7f;
    mutable unsigned long _lastBatterySampleMs = 0;
};