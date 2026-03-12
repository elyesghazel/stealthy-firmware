#include "PowerManager.h"
#include <esp_sleep.h>

static const int BATTERY_ADC_PIN = 9;
static constexpr float ADC_REF_VOLTAGE = 3.3f;
static constexpr float VOLTAGE_DIVIDER_RATIO = 2.0f;
static constexpr float CALIBRATION_FACTOR = 1.08f; // tweak with multimeter later

void PowerManager::begin(unsigned long sleepTimeoutMs) {
    _sleepTimeoutMs = sleepTimeoutMs;
    _lastActivityMs = millis();
    _shouldSleep = false;

    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
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

bool PowerManager::isUsbPowered() const {
    // placeholder for later if you add VBUS detect
    return false;
}

float PowerManager::readBatteryVoltage() const {
    const unsigned long now = millis();

    // only resample every 2s
    if (now - _lastBatterySampleMs < 2000) {
        return _cachedBatteryVoltage;
    }

    const int samples = 32;
    uint32_t total = 0;

    for (int i = 0; i < samples; i++) {
        total += analogRead(BATTERY_ADC_PIN);
        delay(2);
    }

    float raw = total / (float)samples;
    float adcVoltage = (raw / 4095.0f) * ADC_REF_VOLTAGE;
    float measuredVoltage = adcVoltage * VOLTAGE_DIVIDER_RATIO * CALIBRATION_FACTOR;

    // simple low-pass filter
    _cachedBatteryVoltage = (_cachedBatteryVoltage * 0.7f) + (measuredVoltage * 0.3f);
    _lastBatterySampleMs = now;

    return _cachedBatteryVoltage;
}

void PowerManager::enterDeepSleep() {
    Serial.println("[Power] entering deep sleep");
    delay(100);
    esp_deep_sleep_start();
}