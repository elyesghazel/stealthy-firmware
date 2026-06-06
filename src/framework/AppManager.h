#pragma once
#include "framework/IApp.h"
#include "framework/AppContext.h"

class AppManager {
public:
    void begin(IApp* initialApp, AppContext* context);
    void switchTo(IApp* nextApp);

    void handleButton(const ButtonEvent& event);
    void update();
    void render(DisplayManager& display);

    IApp* currentApp() const;

    AppContext* context() const;

private:
    IApp* _currentApp = nullptr;

    AppContext* _context = nullptr;
};