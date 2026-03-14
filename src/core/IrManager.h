#pragma once

#include "../drivers/IrDriver.h"

class IrManager {
public:
    explicit IrManager(IrDriver* driver);

    bool begin();

    void startRecording();
    void update();

    bool isRecording() const;
    bool hasLastCapture() const;
    bool hasNewCapture() const;

    const IrCapture& lastCapture() const;
    void clearNewCaptureFlag();

    bool replayLast();
    bool setLastCapture(const IrCapture& capture);

private:
    IrDriver* _driver = nullptr;
    IrCapture _lastCapture;
    bool _recording = false;
    bool _newCapture = false;
};