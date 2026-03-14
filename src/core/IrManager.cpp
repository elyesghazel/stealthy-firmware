#include "IrManager.h"

IrManager::IrManager(IrDriver* driver)
    : _driver(driver) {
}

bool IrManager::begin() {
    if (_driver == nullptr) {
        return false;
    }

    return _driver->begin();
}

void IrManager::startRecording() {
    if (_driver == nullptr) {
        return;
    }

    _recording = true;
    _newCapture = false;
    _driver->startReceive();

    Serial.println("[IR Manager] recording started");
}

void IrManager::update() {
    if (!_recording || _driver == nullptr) {
        return;
    }

    if (_driver->available()) {
        _lastCapture = _driver->readCapture();
        _recording = false;
        _newCapture = _lastCapture.valid;

        Serial.println("[IR Manager] recording finished");
    }
}

bool IrManager::isRecording() const {
    return _recording;
}

bool IrManager::hasLastCapture() const {
    return _lastCapture.valid;
}

bool IrManager::hasNewCapture() const {
    return _newCapture;
}

const IrCapture& IrManager::lastCapture() const {
    return _lastCapture;
}

void IrManager::clearNewCaptureFlag() {
    _newCapture = false;
}

bool IrManager::replayLast() {
    if (_driver == nullptr || !_lastCapture.valid) {
        return false;
    }

    return _driver->send(_lastCapture);
}