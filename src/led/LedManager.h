#pragma once

class LedDriver;

class LedManager {
public:
    explicit LedManager(LedDriver* driver);

    bool begin();
    void update();

    void setIdle();

    void showBooting();
    void showSuccess();
    void showIrTransmit();

private:
    enum class Mode {
        Idle,
        Booting,
        SuccessPulse,
        IrTransmitPulse
    };

    LedDriver* _driver = nullptr;
    Mode _mode = Mode::Idle;

    unsigned long _modeStartedMs = 0;
    unsigned long _lastBlinkMs = 0;
    bool _bootBlueOn = false;

    void enterMode(Mode mode);
};