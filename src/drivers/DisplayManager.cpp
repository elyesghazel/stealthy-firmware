#include "DisplayManager.h"
#include "core/PowerManager.h"

#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#define EPD_MOSI 4
#define EPD_SCK  5
#define EPD_CS   6
#define EPD_DC   7
#define EPD_RST  15
#define EPD_BUSY 16

#define DISPLAY_ROTATION 3

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
    GxEPD2_213_B74(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

void DisplayManager::begin(bool isColdBoot) {
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    display.init(115200, isColdBoot, 2, false);
    display.setRotation(DISPLAY_ROTATION);
    display.setTextColor(GxEPD_BLACK);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());

    Serial.println("[DisplayManager] Initialized");
}

void DisplayManager::attachPowerManager(PowerManager* powerManager) {
    _powerManager = powerManager;
}

void DisplayManager::prepareForSleep() {
    // put display to sleep to save power, if supported
    display.hibernate();
    Serial.println("[DisplayManager] Display hibernated for sleep");
}

void DisplayManager::startFullWindowDraw() {
    display.setFullWindow();
    display.firstPage();
}

void DisplayManager::startPartialWindowDraw(int x, int y, int w, int h) {
    display.setPartialWindow(x, y, w, h);
    display.firstPage();
}

bool DisplayManager::nextPage() {
    return display.nextPage();
}

void DisplayManager::fillWhite() {
    display.fillScreen(GxEPD_WHITE);
}

void DisplayManager::fillRectWhite(int x, int y, int w, int h) {
    display.fillRect(x, y, w, h, GxEPD_WHITE);
}

void DisplayManager::setDefaultFont() {
    display.setFont();
}

void DisplayManager::setTitleFont() {
    display.setFont(&FreeMonoBold9pt7b);
}

void DisplayManager::setTextBlack() {
    display.setTextColor(GxEPD_BLACK);
}

void DisplayManager::setTextWhite() {
    display.setTextColor(GxEPD_WHITE);
}

void DisplayManager::drawText(int x, int y, const char* text) {
    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::drawText(int x, int y, const String& text) {
    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::fillRect(int x, int y, int w, int h) {
    display.fillRect(x, y, w, h, GxEPD_BLACK);
}

void DisplayManager::drawRect(int x, int y, int w, int h) {
    display.drawRect(x, y, w, h, GxEPD_BLACK);
}

void DisplayManager::getTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    display.getTextBounds(text, x, y, x1, y1, w, h);
}

int DisplayManager::width() const {
    return display.width();
}

int DisplayManager::height() const {
    return display.height();
}

int DisplayManager::batteryBarsFromVoltage(float voltage) const {
    const float upMargin = 0.04f;
    const float downMargin = 0.06f;

    if (_lastBatteryBars < 0) {
        if (voltage >= 4.05f) _lastBatteryBars = 4;
        else if (voltage >= 3.90f) _lastBatteryBars = 3;
        else if (voltage >= 3.75f) _lastBatteryBars = 2;
        else if (voltage >= 3.55f) _lastBatteryBars = 1;
        else _lastBatteryBars = 0;
        return _lastBatteryBars;
    }

    switch (_lastBatteryBars) {
        case 4:
            if (voltage < 4.05f - downMargin) _lastBatteryBars = 3;
            break;
        case 3:
            if (voltage >= 4.05f + upMargin) _lastBatteryBars = 4;
            else if (voltage < 3.90f - downMargin) _lastBatteryBars = 2;
            break;
        case 2:
            if (voltage >= 3.90f + upMargin) _lastBatteryBars = 3;
            else if (voltage < 3.75f - downMargin) _lastBatteryBars = 1;
            break;
        case 1:
            if (voltage >= 3.75f + upMargin) _lastBatteryBars = 2;
            else if (voltage < 3.55f - downMargin) _lastBatteryBars = 0;
            break;
        case 0:
        default:
            if (voltage >= 3.55f + upMargin) _lastBatteryBars = 1;
            break;
    }

    return _lastBatteryBars;
}

void DisplayManager::drawBatteryIcon(int x, int y, float voltage) {
    const int bodyW = 18;
    const int bodyH = 9;
    const int tipW = 2;
    const int tipH = 4;

    drawRect(x, y, bodyW, bodyH);
    fillRect(x + bodyW, y + 2, tipW, tipH);

    int bars = batteryBarsFromVoltage(voltage);

    for (int i = 0; i < 4; i++) {
        if (i < bars) {
            fillRect(x + 2 + i * 4, y + 2, 3, 5);
        }
    }
}

void DisplayManager::drawStatusBar() {
    setTextBlack();

    // use the title font for the app name so it feels more deliberate
    setTitleFont();
    drawText(4, 14, "Stealthy");

    float voltage = 0.0f;
    if (_powerManager) {
        voltage = _powerManager->readBatteryVoltage();
    }

    // battery icon only
    drawBatteryIcon(width() - 24, 4, voltage);

    // divider line
    fillRect(0, 19, width(), 1);

    // switch back to default for content afterwards
    setDefaultFont();
}