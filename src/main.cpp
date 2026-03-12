#include <Arduino.h>

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

    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    displayManager.begin();
    powerManager.begin(300000); // 5 min for development
    displayManager.attachPowerManager(&powerManager);

    buttonManager.begin();

    startScreenApp.setup(&appManager, &mainMenuApp);
    badgeApp.setup(&appManager, &mainMenuApp);
    aboutApp.setup(&appManager, &mainMenuApp);
    settingsApp.setup(&appManager, &mainMenuApp, &deviceSettings);

    mainMenuApp.setup(
        &appManager,
        &badgeApp,
        &settingsApp,
        &aboutApp
    );

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
        Serial.printf("Battery: %.2f V\n", powerManager.readBatteryVoltage());
    }

    if (powerManager.shouldSleep()) {
        powerManager.enterDeepSleep();
    }

    delay(50);
}