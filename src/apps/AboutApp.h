#pragma once
#include "core/IApp.h"

class AppManager;
class IApp;

class AboutApp : public IApp {
public:
    AboutApp();

    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    AppManager* _appManager = nullptr;
    IApp* _returnApp = nullptr;
    bool _needsRender = true;
};