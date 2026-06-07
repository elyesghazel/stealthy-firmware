#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_system.h>

#include "input/ButtonManager.h"
#include "display/DisplayManager.h"
#include "storage/FileSystemDriver.h"
#include "ir/IrDriver.h"
#include "led/LedDriver.h"

#include "framework/AppManager.h"
#include "power/PowerManager.h"
#include "storage/StorageManager.h"
#include "framework/AppContext.h"
#include "ir/IrManager.h"
#include "led/LedManager.h"
#include "portal/PortalManager.h"
#include "wifi/WifiSpammer.h"
#include "wifi/WifiDeauth.h"
#include "wifi/WifiKarma.h"

#include "DeviceSettings.h"

#include "apps/StartScreenApp.h"
#include "apps/BadgeApp.h"
#include "apps/MainMenuApp.h"
#include "apps/AboutApp.h"
#include "apps/IrToolsApp.h"
#include "apps/SettingsApp.h"
#include "apps/PortalApp.h"
#include "apps/WifiToolsApp.h"
#include "apps/WifiSpammerApp.h"
#include "apps/WifiDeauthApp.h"
#include "apps/TotpApp.h"
#include "totp/TotpManager.h"

// Pins
static const int PIN_BTN_UP     = 10;
static const int PIN_BTN_DOWN   = 11;
static const int PIN_BTN_SELECT = 12;
static const int PIN_BTN_BACK   = 13;
static const int LED_BLUE       = 2;
static const int LED_GREEN      = 1;

static uint32_t _autoStartPortalAt   = 0;
static uint32_t _lastBatteryCheckMs  = 0;

// Drivers & managers
AppContext     appContext;
FileSystemDriver fileSystemDriver;
IrDriver       irDriver;
LedDriver      ledDriver(LED_BLUE, LED_GREEN);

StorageManager storageManager(&fileSystemDriver);
DisplayManager displayManager;
PowerManager   powerManager;
AppManager     appManager;
IrManager      irManager(&irDriver);
LedManager     ledManager(&ledDriver);
ButtonManager  buttonManager(PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_SELECT, PIN_BTN_BACK);
WifiSpammer    wifiSpammer;
WifiDeauth     wifiDeauth;
WifiKarma      wifiKarma;
DeviceSettings deviceSettings;
TotpManager    totpManager;

PortalManager  portalManager(&storageManager, &irManager, &powerManager, &wifiKarma, &deviceSettings, &totpManager);

// Apps
TotpApp        totpApp;
StartScreenApp startScreenApp;
BadgeApp       badgeApp;
MainMenuApp    mainMenuApp;
AboutApp       aboutApp;
SettingsApp    settingsApp;
IrToolsApp     irToolsApp;
PortalApp      portalApp;
WifiToolsApp   wifiToolsApp;
WifiSpammerApp wifiSpammerApp;
WifiDeauthApp  wifiDeauthApp;

void initFileSystem() {
    if (!storageManager.begin()) {
        Serial.println("[Storage] init failed");
    } else {
        Serial.println("[Storage] ready");
        Serial.println("[Storage] badge: " + storageManager.getBadgeName());
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

    displayManager.begin(!wokeFromDeepSleep);
    powerManager.begin(300000);
    displayManager.attachPowerManager(&powerManager);

    Serial.printf("Wake cause: %d\n", wakeCause);
    rtc_gpio_deinit(GPIO_NUM_12);
    buttonManager.begin();

    if (!irManager.begin()) Serial.println("[IR] init failed");
    else                    Serial.println("[IR] ready");

    appContext.display  = &displayManager;
    appContext.buttons  = &buttonManager;
    appContext.power    = &powerManager;
    appContext.storage  = &storageManager;
    appContext.ir       = &irManager;
    appContext.leds     = &ledManager;
    appContext.portal   = &portalManager;
    appContext.spammer  = &wifiSpammer;
    appContext.deauth   = &wifiDeauth;
    appContext.karma    = &wifiKarma;
    appContext.totp     = &totpManager;

    initFileSystem();
    totpManager.begin();

    // Load persisted settings; apply sleep timeout immediately
    storageManager.loadDeviceSettings(deviceSettings);
    {
        static const unsigned long timeouts[] = {10000, 30000, 60000, 180000, 300000, 600000};
        int idx = constrain(deviceSettings.sleepTimeoutIndex, 0, 5);
        powerManager.setSleepTimeout(timeouts[idx]);
    }

    if (storageManager.getPortalAutostart()) {
        _autoStartPortalAt = millis() + 2500;
        Serial.println("[Boot] portal autostart scheduled");
    }

    // App setup
    startScreenApp.setup(&appManager, &mainMenuApp);
    badgeApp.setup(&appManager, &mainMenuApp);
    aboutApp.setup(&appManager, &mainMenuApp);
    settingsApp.setup(&appManager, &mainMenuApp, &deviceSettings);
    irToolsApp.setup(&appManager, &mainMenuApp);
    portalApp.setup(&appManager, &mainMenuApp);
    wifiToolsApp.setup(&appManager, &mainMenuApp, &wifiSpammerApp, &wifiDeauthApp);
    wifiSpammerApp.setup(&appManager, &wifiToolsApp);
    wifiDeauthApp.setup(&appManager, &wifiToolsApp);
    totpApp.setup(&appManager, &mainMenuApp);

    mainMenuApp.setup(
        &appManager,
        &badgeApp, &settingsApp, &aboutApp,
        &irToolsApp, &portalApp, &wifiToolsApp, &totpApp
    );

    IApp* firstApp = deviceSettings.showStartScreen ? (IApp*)&startScreenApp : (IApp*)&badgeApp;
    appManager.begin(firstApp, &appContext);
    appManager.setHomeApp(&mainMenuApp);
    ledManager.showSuccess();
}

void loop() {
    if (_autoStartPortalAt > 0 && millis() >= _autoStartPortalAt) {
        _autoStartPortalAt = 0;
        if (!portalManager.isRunning()) {
            portalManager.begin();
            Serial.println("[Boot] portal autostarted");
        }
    }

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
        Serial.printf("Battery: %.2f V\n", powerManager.readBatteryVoltage());
    }

    // Battery low warning: check every 30 s, trigger LED below 10 %
    if (millis() - _lastBatteryCheckMs >= 30000) {
        _lastBatteryCheckMs = millis();
        if (powerManager.batteryPercent() <= 10) {
            ledManager.showBatteryLow();
        }
    }

    if (powerManager.shouldSleep()) {
        displayManager.prepareForSleep();
        powerManager.enterDeepSleep();
    }
    delay(10);
}
