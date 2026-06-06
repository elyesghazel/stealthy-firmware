#include "LedDriver.h"

LedDriver::LedDriver(int bluePin, int greenPin)
    : _bluePin(bluePin),
      _greenPin(greenPin) {
}

void LedDriver::begin() {
    pinMode(_bluePin, OUTPUT);
    pinMode(_greenPin, OUTPUT);
    setAllOff();
}

void LedDriver::setBlue(bool on) {
    _blueOn = on;
    digitalWrite(_bluePin, on ? HIGH : LOW);
}

void LedDriver::setGreen(bool on) {
    _greenOn = on;
    digitalWrite(_greenPin, on ? HIGH : LOW);
}

void LedDriver::setAllOff() {
    setBlue(false);
    setGreen(false);
}

bool LedDriver::blueState() const {
    return _blueOn;
}

bool LedDriver::greenState() const {
    return _greenOn;
}