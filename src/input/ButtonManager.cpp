#include "ButtonManager.h"

ButtonManager::ButtonManager(int upPin, int downPin, int selectPin, int backPin)
    : _upPin(upPin),
      _downPin(downPin),
      _selectPin(selectPin),
      _backPin(backPin),
      _queueHead(0),
      _queueTail(0) {
}

void ButtonManager::begin() {
    pinMode(_upPin, INPUT_PULLUP);
    pinMode(_downPin, INPUT_PULLUP);
    pinMode(_selectPin, INPUT_PULLUP);
    pinMode(_backPin, INPUT_PULLUP);

    _up.pin = _upPin;
    _up.id = ButtonId::Up;

    _down.pin = _downPin;
    _down.id = ButtonId::Down;

    _select.pin = _selectPin;
    _select.id = ButtonId::Select;

    _back.pin = _backPin;
    _back.id = ButtonId::Back;
}

bool ButtonManager::readPressed(int pin) {
    return digitalRead(pin) == LOW;
}

void ButtonManager::pushEvent(ButtonId id, ButtonAction action) {
    int nextTail = (_queueTail + 1) % MAX_EVENTS;
    if (nextTail == _queueHead) {
        return;
    }

    _queue[_queueTail] = {id, action};
    _queueTail = nextTail;
}

void ButtonManager::updateButton(ButtonState& button, unsigned long now) {
    bool rawNow = readPressed(button.pin);

    if (rawNow != button.lastRawPressed) {
        button.lastRawPressed = rawNow;
        button.lastDebounceMs = now;
    }

    if ((now - button.lastDebounceMs) >= DEBOUNCE_MS) {
        if (rawNow != button.stablePressed) {
            button.stablePressed = rawNow;

            if (button.stablePressed) {
                button.pressedSinceMs = now;
                button.lastRepeatMs = now;
                button.longPressFired = false;

                pushEvent(button.id, ButtonAction::Press);
            } else {
                button.longPressFired = false;
                button.lastRepeatMs = 0;
                button.pressedSinceMs = 0;
            }
        }
    }

    if (button.stablePressed) {
        if (!button.longPressFired &&
            (now - button.pressedSinceMs) >= LONG_PRESS_MS) {
            button.longPressFired = true;
            pushEvent(button.id, ButtonAction::LongPress);
        }

        const bool allowRepeat =
            (button.id == ButtonId::Up || button.id == ButtonId::Down);

        if (allowRepeat &&
            rawNow &&  // important: must still be physically pressed now
            (now - button.pressedSinceMs) >= REPEAT_START_MS &&
            (now - button.lastRepeatMs) >= REPEAT_INTERVAL_MS) {
            button.lastRepeatMs = now;
            pushEvent(button.id, ButtonAction::Repeat);
        }
    }
}

void ButtonManager::update() {
    unsigned long now = millis();

    updateButton(_up, now);
    updateButton(_down, now);
    updateButton(_select, now);
    updateButton(_back, now);
}

bool ButtonManager::hasEvent() const {
    return _queueHead != _queueTail;
}

ButtonEvent ButtonManager::getNextEvent() {
    if (!hasEvent()) {
        return {ButtonId::Up, ButtonAction::Press};
    }

    ButtonEvent event = _queue[_queueHead];
    _queueHead = (_queueHead + 1) % MAX_EVENTS;
    return event;
}