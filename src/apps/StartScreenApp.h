#pragma once

#include "framework/IApp.h"

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
    enum class RenderMode {
        Full,
        Partial
    };

    void requestFullRender();
    void requestPartialRender();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);

    void drawBootContent(DisplayManager& display);
    void drawProgressBar(DisplayManager& display, int percent);

    AppManager* _appManager = nullptr;
    IApp* _nextApp = nullptr;

    bool _needsRender = true;
    RenderMode _renderMode = RenderMode::Full;

    int _stage = 0;
    unsigned long _stageStartedMs = 0;
};