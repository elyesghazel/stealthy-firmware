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
    static const int MAX_EVENTS = 8;

    int _upPin;
    int _downPin;
    int _selectPin;
    int _backPin;

    bool _lastUpState;
    bool _lastDownState;
    bool _lastSelectState;
    bool _lastBackState;

    ButtonEvent _queue[MAX_EVENTS];
    int _queueHead;
    int _queueTail;

    void pushEvent(ButtonId id, ButtonAction action);
    bool readPressed(int pin);
};