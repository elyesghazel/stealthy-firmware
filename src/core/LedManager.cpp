#include "LedManager.h"
#include "../drivers/LedDriver.h"
#include <Arduino.h>

LedManager::LedManager(LedDriver* driver)
    : _driver(driver) {
}

bool LedManager::begin() {
    if (_driver == nullptr) {
        return false;
    }

    _driver->setAllOff();
    _mode = Mode::Idle;
    _modeStartedMs = millis();
    _lastBlinkMs = millis();
    return true;
}

void LedManager::enterMode(Mode mode) {
    _mode = mode;
    _modeStartedMs = millis();
    _lastBlinkMs = millis();
}

void LedManager::setIdle() {
    if (_driver == nullptr) {
        return;
    }

    _driver->setAllOff();
    _mode = Mode::Idle;
}

void LedManager::showBooting() {
    if (_driver == nullptr) {
        return;
    }

    _driver->setGreen(false);
    _driver->setBlue(true);
    _bootBlueOn = true;
    enterMode(Mode::Booting);
}

void LedManager::showSuccess() {
    if (_driver == nullptr) {
        return;
    }

    _driver->setBlue(false);
    _driver->setGreen(true);
    enterMode(Mode::SuccessPulse);
}

void LedManager::showIrTransmit() {
    if (_driver == nullptr) {
        return;
    }

    _driver->setGreen(false);
    _driver->setBlue(true);
    enterMode(Mode::IrTransmitPulse);
}

void LedManager::update() {
    if (_driver == nullptr) {
        return;
    }

    const unsigned long now = millis();

    switch (_mode) {
        case Mode::Idle:
            break;

        case Mode::Booting:
            if (now - _lastBlinkMs >= 180) {
                _lastBlinkMs = now;
                _bootBlueOn = !_bootBlueOn;
                _driver->setBlue(_bootBlueOn);
            }
            break;

        case Mode::SuccessPulse:
            if (now - _modeStartedMs >= 180) {
                _driver->setGreen(false);
                _mode = Mode::Idle;
            }
            break;

        case Mode::IrTransmitPulse:
            if (now - _modeStartedMs >= 40) {
                _driver->setBlue(false);
                _mode = Mode::Idle;
            }
            break;
    }
}