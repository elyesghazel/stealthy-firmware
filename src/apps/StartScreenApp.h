#pragma once
#include "core/IApp.h"

class AppManager;
class IApp;

class StartScreenApp : public IApp {
public:
    StartScreenApp();

    void setup(AppManager* appManager, IApp* nextApp);
    
    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    AppManager* _appManager = nullptr;
    IApp* _nextApp = nullptr;

    bool _needsRender = true;
};