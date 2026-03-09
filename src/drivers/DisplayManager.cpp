#include "DisplayManager.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#define EPD_MOSI 4
#define EPD_SCK  5
#define EPD_CS   6
#define EPD_DC   7
#define EPD_RST  15
#define EPD_BUSY 16

#define displayRotation 3

GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

void DisplayManager::begin() {
    // Initialize the display here
    
    display.init(115200, true, 2, false);
    display.setRotation(displayRotation);
    display.setFont(&FreeMonoBold9pt7b);
    Serial.println("[DisplayManager] Initialized");
}

void DisplayManager::clear() {
    display.clearScreen();
    display.fillScreen(GxEPD_WHITE);

    Serial.println("[DisplayManager] Screen cleared");
}

void DisplayManager::drawText(int x, int y, const String& text) {
    display.setCursor(x, y);
    display.println(text);
    Serial.printf("[Display] text (%d,%d): %s\n", x, y, text.c_str());
}


void DisplayManager::drawMenu(const String items[], int count, int selectedIndex) {
    for (int i = 0; i < count; i++) {
        String prefix = (i == selectedIndex) ? "> " : "  ";
        drawText(0, 20 + i * 12, prefix + items[i]);
    }
}

void DisplayManager::updateFull() {
    // Full refresh call here
    display.refresh();
    Serial.println("[Display] full refresh");
}

void DisplayManager::updatePartial() {
    // Partial refresh call here
    display.setPartialWindow(0, 0, display.width(), display.height());
    display.refresh(true);
    Serial.println("[Display] partial refresh");
}