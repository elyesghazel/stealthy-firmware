#pragma once
#include "core/IApp.h"

class AppManager;
class IApp;

class BadgeApp : public IApp {
public:
    BadgeApp();

    void setup(AppManager* appManager, IApp* mainMenuApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    AppManager* _appManager = nullptr;
    IApp* _mainMenuApp = nullptr;

    bool _needsRender = true;

    const char* _name = "Your Name";
    const char* _status = "Online";
};