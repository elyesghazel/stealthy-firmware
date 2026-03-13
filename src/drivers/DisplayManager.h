#pragma once
#include <Arduino.h>

class PowerManager;

class DisplayManager {
public:
    void begin(bool isColdBoot);
    void attachPowerManager(PowerManager* powerManager);
    void prepareForSleep();

    void startFullWindowDraw();
    void startPartialWindowDraw(int x, int y, int w, int h);
    bool nextPage();

    void fillWhite();

    void setDefaultFont();
    void setTitleFont();

    void setTextBlack();
    void setTextWhite();

    void drawText(int x, int y, const char* text);
    void drawText(int x, int y, const String& text);

    void fillRect(int x, int y, int w, int h);
    void fillRectWhite(int x, int y, int w, int h);
    void drawRect(int x, int y, int w, int h);

    void getTextBounds(const char* text, int x, int y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

    int width() const;
    int height() const;

    void drawStatusBar();

private:
    PowerManager* _powerManager = nullptr;

    int batteryBarsFromVoltage(float voltage) const;
    void drawBatteryIcon(int x, int y, float voltage);
    mutable int _lastBatteryBars = -1;
};