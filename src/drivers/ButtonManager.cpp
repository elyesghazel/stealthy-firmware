#include "ButtonManager.h"

ButtonManager::ButtonManager(int upPin, int downPin, int selectPin, int backPin)
    : _upPin(upPin),
      _downPin(downPin),
      _selectPin(selectPin),
      _backPin(backPin),
      _lastUpState(false),
      _lastDownState(false),
      _lastSelectState(false),
      _lastBackState(false),
      _queueHead(0),
      _queueTail(0) {}

void ButtonManager::begin() {
    pinMode(_upPin, INPUT_PULLUP);
    pinMode(_downPin, INPUT_PULLUP);
    pinMode(_selectPin, INPUT_PULLUP);
    pinMode(_backPin, INPUT_PULLUP);
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

void ButtonManager::update() {
    bool upNow = readPressed(_upPin);
    bool downNow = readPressed(_downPin);
    bool selectNow = readPressed(_selectPin);
    bool backNow = readPressed(_backPin);

    if (upNow && !_lastUpState) {
        pushEvent(ButtonId::Up, ButtonAction::Press);
    }
    if (downNow && !_lastDownState) {
        pushEvent(ButtonId::Down, ButtonAction::Press);
    }
    if (selectNow && !_lastSelectState) {
        pushEvent(ButtonId::Select, ButtonAction::Press);
    }
    if (backNow && !_lastBackState) {
        pushEvent(ButtonId::Back, ButtonAction::Press);
    }

    _lastUpState = upNow;
    _lastDownState = downNow;
    _lastSelectState = selectNow;
    _lastBackState = backNow;
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