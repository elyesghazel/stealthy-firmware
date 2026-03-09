#pragma once
#include <Arduino.h>

class DisplayManager {
public:
    void begin();
    void clear();
    void drawText(int x, int y, const String& text);
    void drawMenu(const String items[], int count, int selectedIndex);
    void updateFull();
    void updatePartial();
};