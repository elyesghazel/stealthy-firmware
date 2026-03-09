#pragma once

class DisplayManager;

enum class ButtonId {
    Up,
    Down,
    Select,
    Back
};

enum class ButtonAction {
    Press,
    LongPress
};

struct ButtonEvent {
    ButtonId id;
    ButtonAction action;
};

class IApp {
public:
    virtual ~IApp() {}

    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void handleButton(const ButtonEvent& event) = 0;
    virtual void update() = 0;
    virtual void render(DisplayManager& display) = 0;
};