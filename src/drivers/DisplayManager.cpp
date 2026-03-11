#include "DisplayManager.h"
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

void DisplayManager::begin() {
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    display.init(115200, true, 2, false);
    display.setRotation(DISPLAY_ROTATION);
    display.setTextColor(GxEPD_BLACK);

    Serial.println("[DisplayManager] Initialized");
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


void DisplayManager::getTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    display.getTextBounds(text, x, y, x1, y1, w, h);
}