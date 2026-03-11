#include <Arduino.h>

#include "core/AppManager.h"
#include "core/PowerManager.h"
#include "drivers/ButtonManager.h"
#include "drivers/DisplayManager.h"
#include "apps/BadgeApp.h"
#include "apps/MainMenuApp.h"

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

BadgeApp badgeApp;
MainMenuApp mainMenuApp;

void setup() {
    Serial.begin(115200);

    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    displayManager.begin();
    buttonManager.begin();
    powerManager.begin(30000);

    badgeApp.setup(&appManager, &mainMenuApp);
    mainMenuApp.setup(&appManager, &badgeApp);

    appManager.begin(&badgeApp);
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

    if (powerManager.shouldSleep()) {
        powerManager.enterDeepSleep();
    }

    delay(50);
}