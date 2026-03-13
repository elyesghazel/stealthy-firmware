#include "PowerManager.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>


static const int BATTERY_ADC_PIN = 9;
static const gpio_num_t WAKE_BTN = GPIO_NUM_12;
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

    if (now - _lastBatterySampleMs < 1000) {
        return _cachedBatteryVoltage;
    }

    static bool initialized = false;

    const int samples = 20;
    uint16_t readings[samples];

    for (int i = 0; i < samples; i++) {
        readings[i] = analogRead(BATTERY_ADC_PIN);
        delay(2);
    }

    uint16_t minVal = readings[0];
    uint16_t maxVal = readings[0];
    uint32_t total = 0;

    for (int i = 0; i < samples; i++) {
        if (readings[i] < minVal) minVal = readings[i];
        if (readings[i] > maxVal) maxVal = readings[i];
        total += readings[i];
    }

    // discard one min and one max to reduce spikes
    total -= minVal;
    total -= maxVal;

    float raw = total / float(samples - 2);
    float adcVoltage = (raw / 4095.0f) * ADC_REF_VOLTAGE;
    float measuredVoltage = adcVoltage * VOLTAGE_DIVIDER_RATIO * CALIBRATION_FACTOR;

    if (!initialized) {
        _cachedBatteryVoltage = measuredVoltage;
        initialized = true;
    } else {
        // stronger smoothing = more stable UI
        _cachedBatteryVoltage = (_cachedBatteryVoltage * 0.85f) + (measuredVoltage * 0.15f);
    }

    _lastBatterySampleMs = now;
    return _cachedBatteryVoltage;
}

int PowerManager::batteryPercent() const {
    float v = readBatteryVoltage();

    // clamp to realistic 1S LiPo range
    if (v >= 4.20f) return 100;
    if (v <= 3.30f) return 0;

    // simple piecewise approximation for LiPo discharge curve
    if (v >= 4.10f) return 90 + int((v - 4.10f) / 0.10f * 10.0f);   // 4.10 - 4.20
    if (v >= 4.00f) return 80 + int((v - 4.00f) / 0.10f * 10.0f);   // 4.00 - 4.10
    if (v >= 3.90f) return 60 + int((v - 3.90f) / 0.10f * 20.0f);   // 3.90 - 4.00
    if (v >= 3.80f) return 40 + int((v - 3.80f) / 0.10f * 20.0f);   // 3.80 - 3.90
    if (v >= 3.70f) return 25 + int((v - 3.70f) / 0.10f * 15.0f);   // 3.70 - 3.80
    if (v >= 3.60f) return 10 + int((v - 3.60f) / 0.10f * 15.0f);   // 3.60 - 3.70
    if (v >= 3.50f) return 5  + int((v - 3.50f) / 0.10f * 5.0f);    // 3.50 - 3.60

    return int((v - 3.30f) / 0.20f * 5.0f); // 3.30 - 3.50
}

#include <esp_sleep.h>


void PowerManager::enterDeepSleep() {
    Serial.println("[Power] preparing deep sleep");

    pinMode((int)WAKE_BTN, INPUT_PULLUP);
    Serial.printf("GPIO12 before sleep = %d\n", digitalRead((int)WAKE_BTN));

    // Configure GPIO12 as RTC IO with pull-up held during sleep
    rtc_gpio_init(WAKE_BTN);
    rtc_gpio_set_direction(WAKE_BTN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(WAKE_BTN);
    rtc_gpio_pulldown_dis(WAKE_BTN);

    // Keep RTC peripheral domain on so the RTC pull-up stays active
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    esp_sleep_enable_ext1_wakeup(1ULL << WAKE_BTN, ESP_EXT1_WAKEUP_ANY_LOW);

    delay(100);
    Serial.println("[Power] entering deep sleep");
    Serial.flush();

    esp_deep_sleep_start();
}