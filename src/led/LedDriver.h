#pragma once

#include <Arduino.h>

class LedDriver {
public:
    LedDriver(int bluePin, int greenPin);

    void begin();

    void setBlue(bool on);
    void setGreen(bool on);

    void setAllOff();

    bool blueState() const;
    bool greenState() const;

private:
    int _bluePin;
    int _greenPin;

    bool _blueOn = false;
    bool _greenOn = false;
};