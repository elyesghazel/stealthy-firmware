#include "PowerManager.h"
#include <esp_sleep.h>

void PowerManager::begin(unsigned long sleepTimeoutMs) {
    _sleepTimeoutMs = sleepTimeoutMs;
    _lastActivityMs = millis();
    _shouldSleep = false;
}

void PowerManager::notifyUserActivity() {
    _lastActivityMs = millis();
    _shouldSleep = false;
}

void PowerManager::update() {
    if (millis() - _lastActivityMs >= _sleepTimeoutMs) {
        _shouldSleep = true;
    }
}

bool PowerManager::shouldSleep() const {
    return _shouldSleep;
}

float PowerManager::readBatteryVoltage() const {
    // Replace with ADC logic later
    return 3.95f;
}

void PowerManager::enterDeepSleep() {
    Serial.println("[Power] entering deep sleep");

    // Example: wake on external GPIO later
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_X, 0);

    delay(100);
    esp_deep_sleep_start();
}