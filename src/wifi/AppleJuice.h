#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

enum class AppleMode : uint8_t { All, AudioOnly, SetupOnly };

class AppleJuice {
public:
    bool begin(AppleMode mode = AppleMode::All);
    void stop();
    void update();   // display refresh only — BLE runs in its own task

    bool isRunning()    const { return _running; }
    int  sentCount()    const { return _sentCount; }
    AppleMode mode()    const { return _mode; }
    const char* currentName() const;

    void setMode(AppleMode m);

private:
    static void taskFunc(void* param);
    void        advertiseOne();

    int  nextIndex(int from) const;
    bool indexMatchesMode(int idx) const;

    volatile bool  _running    = false;
    volatile int   _sentCount  = 0;
    volatile int   _currentIdx = 0;
    AppleMode      _mode       = AppleMode::All;
    TaskHandle_t   _task       = nullptr;
};
