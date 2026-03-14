#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_system.h>


#include "drivers/ButtonManager.h"
#include "drivers/DisplayManager.h"
#include "drivers/FileSystemDriver.h"
#include "drivers/IrDriver.h"
#include "drivers/LedDriver.h"

#include "core/AppManager.h"
#include "core/PowerManager.h"
#include "core/StorageManager.h"
#include "core/AppContext.h"
#include "core/IrManager.h"
#include "core/LedManager.h"
#include "core/PortalManager.h"


#include "DeviceSettings.h"

#include "apps/StartScreenApp.h"
#include "apps/BadgeApp.h"
#include "apps/MainMenuApp.h"
#include "apps/AboutApp.h"
#include "apps/IrToolsApp.h"
#include "apps/SettingsApp.h"
#include "apps/PortalApp.h"


// pins
static const int PIN_BTN_UP = 10;
static const int PIN_BTN_DOWN = 11;
static const int PIN_BTN_SELECT = 12;
static const int PIN_BTN_BACK = 13;

static const int LED_BLUE = 2;
static const int LED_GREEN = 1;

// drivers and managers
AppContext appContext;
FileSystemDriver fileSystemDriver;
IrDriver irDriver;
LedDriver ledDriver(LED_BLUE, LED_GREEN);

StorageManager storageManager(&fileSystemDriver);
DisplayManager displayManager;
PowerManager powerManager;
AppManager appManager;
IrManager irManager(&irDriver);
LedManager ledManager(&ledDriver);

ButtonManager buttonManager(PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_SELECT, PIN_BTN_BACK);
PortalManager portalManager(&storageManager, &irManager, &powerManager);

DeviceSettings deviceSettings;

// apps
StartScreenApp startScreenApp;
BadgeApp badgeApp;
MainMenuApp mainMenuApp;
AboutApp aboutApp;
SettingsApp settingsApp;
IrToolsApp irToolsApp;
PortalApp portalApp;

void initFileSystem() {
    // note: file system gets initialized in StorageManager
    if (!storageManager.begin()) {
        Serial.println("[Storage] init failed");
    } else {
        Serial.println("[Storage] ready");
        Serial.println("[DEBUG Storage]: " + storageManager.getBadgeName());
        Serial.println("[DEBUG Storage]: " + storageManager.getBadgeStatus());
    }
}


void printResetReason() {
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.print("[Boot] Reset reason: ");
    Serial.println((int)reason);
}



void setup() {
    Serial.begin(115200);
    delay(200);
    printResetReason();

    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    bool wokeFromDeepSleep = (wakeCause == ESP_SLEEP_WAKEUP_EXT1);

    ledDriver.begin();
    ledManager.begin();
    ledManager.showBooting();


    displayManager.begin(!wokeFromDeepSleep); // true on cold boot, false after deep sleep wake
    powerManager.begin(300000); // 5 minute inactivity timeout for sleep
    
    displayManager.attachPowerManager(&powerManager);
    printResetReason();


    Serial.printf("Wake cause: %d\n", wakeCause);
    if (wakeCause == ESP_SLEEP_WAKEUP_EXT1) {
        Serial.printf("EXT1 status mask: 0x%llX\n", esp_sleep_get_ext1_wakeup_status());
    }

    rtc_gpio_deinit(GPIO_NUM_12);

    buttonManager.begin();

    if (!irManager.begin()) {
        Serial.println("[IR Manager] failed to initialize");
    } else {
        Serial.println("[IR Manager] ready");
    }

    appContext.display = &displayManager;
    appContext.buttons = &buttonManager;
    appContext.power = &powerManager;
    appContext.storage = &storageManager;
    appContext.ir = &irManager;
    appContext.leds = &ledManager;
    appContext.portal = &portalManager;

    // Initialize filesystem and storage manager
    initFileSystem();

    // app initialization and setup
    startScreenApp.setup(&appManager, &mainMenuApp);
    badgeApp.setup(&appManager, &mainMenuApp);
    aboutApp.setup(&appManager, &mainMenuApp);
    settingsApp.setup(&appManager, &mainMenuApp, &deviceSettings);
    irToolsApp.setup(&appManager, &mainMenuApp);
    portalApp.setup(&appManager, &mainMenuApp);

    // portalManager.begin();

    mainMenuApp.setup(&appManager, &badgeApp, &settingsApp, &aboutApp, &irToolsApp, &portalApp);

    appManager.begin(&startScreenApp, &appContext);

    ledManager.showSuccess();
}

void loop() {
    buttonManager.update();
    powerManager.update();
    ledManager.update();
    portalManager.update();

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
    delay(10);
}