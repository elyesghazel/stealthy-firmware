#pragma once
#include "core/IApp.h"

class AppManager {
public:
    void begin(IApp* initialApp);
    void switchTo(IApp* nextApp);

    void handleButton(const ButtonEvent& event);
    void update();
    void render(DisplayManager& display);

    IApp* currentApp() const;

private:
    IApp* _currentApp = nullptr;
};