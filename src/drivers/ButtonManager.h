#pragma once
#include <Arduino.h>
#include "core/IApp.h"

class ButtonManager {
public:
    ButtonManager(int upPin, int downPin, int selectPin, int backPin);

    void begin();
    void update();

    bool hasEvent() const;
    ButtonEvent getNextEvent();

private:
    static constexpr int MAX_EVENTS = 16;

    static constexpr unsigned long DEBOUNCE_MS = 25;
    static constexpr unsigned long LONG_PRESS_MS = 700;
    static constexpr unsigned long REPEAT_START_MS = 300;
    static constexpr unsigned long REPEAT_INTERVAL_MS = 110;

    struct ButtonState {
        int pin = -1;
        ButtonId id = ButtonId::Up;

        bool lastRawPressed = false;
        bool stablePressed = false;

        unsigned long lastDebounceMs = 0;
        unsigned long pressedSinceMs = 0;
        unsigned long lastRepeatMs = 0;

        bool longPressFired = false;
    };

    int _upPin;
    int _downPin;
    int _selectPin;
    int _backPin;

    ButtonState _up;
    ButtonState _down;
    ButtonState _select;
    ButtonState _back;

    ButtonEvent _queue[MAX_EVENTS];
    int _queueHead;
    int _queueTail;

    void pushEvent(ButtonId id, ButtonAction action);
    static bool readPressed(int pin);
    void updateButton(ButtonState& button, unsigned long now);
};