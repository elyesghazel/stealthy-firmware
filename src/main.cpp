#include <Arduino.h>
#include <driver/rtc_io.h>

#include "core/AppManager.h"
#include "core/PowerManager.h"
#include "drivers/ButtonManager.h"
#include "drivers/DisplayManager.h"

#include "DeviceSettings.h"

#include "apps/StartScreenApp.h"
#include "apps/BadgeApp.h"
#include "apps/MainMenuApp.h"
#include "apps/AboutApp.h"
#include "apps/SettingsApp.h"

// pins
static const int PIN_BTN_UP = 10;
static const int PIN_BTN_DOWN = 11;
static const int PIN_BTN_SELECT = 12;
static const int PIN_BTN_BACK = 13;

static const int LED_BLUE = 2;
static const int LED_GREEN = 1;

DisplayManager displayManager;
PowerManager powerManager;
AppManager appManager;
ButtonManager buttonManager(PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_SELECT, PIN_BTN_BACK);

DeviceSettings deviceSettings;

StartScreenApp startScreenApp;
BadgeApp badgeApp;
MainMenuApp mainMenuApp;
AboutApp aboutApp;
SettingsApp settingsApp;



void setup() {
    Serial.begin(115200);
    delay(200);

    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    bool wokeFromDeepSleep = (wakeCause == ESP_SLEEP_WAKEUP_EXT1);

    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    displayManager.begin(!wokeFromDeepSleep); // true on cold boot, false after deep sleep wake
    powerManager.begin(300000); // 5 minute inactivity timeout for sleep
    displayManager.attachPowerManager(&powerManager);

    Serial.printf("Wake cause: %d\n", wakeCause);
    if (wakeCause == ESP_SLEEP_WAKEUP_EXT1) {
        Serial.printf("EXT1 status mask: 0x%llX\n", esp_sleep_get_ext1_wakeup_status());
    }

    rtc_gpio_deinit(GPIO_NUM_12);

    buttonManager.begin();

    startScreenApp.setup(&appManager, &mainMenuApp);
    badgeApp.setup(&appManager, &mainMenuApp);
    aboutApp.setup(&appManager, &mainMenuApp);
    settingsApp.setup(&appManager, &mainMenuApp, &deviceSettings);

    mainMenuApp.setup(&appManager, &badgeApp, &settingsApp, &aboutApp);

    appManager.begin(&startScreenApp);
}

void loop() {
    buttonManager.update();
    powerManager.update();

    while (buttonManager.hasEvent()) {
        ButtonEvent event = buttonManager.getNextEvent();
        powerManager.notifyUserActivity();
        appManager.handleButton(event);
    }

    appManager.update();
    appManager.render(displayManager);

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 3000) {
        lastPrint = millis();
        Serial.print("Battery: ");
        Serial.print(powerManager.readBatteryVoltage(), 2);
        Serial.println(" V");
    }

    if (powerManager.shouldSleep()) {
        displayManager.prepareForSleep();
        powerManager.enterDeepSleep();
    }

    delay(50);
}