#pragma once

#include "../core/IApp.h"

class AppManager;
class IApp;
class DisplayManager;

class PortalApp : public IApp {
public:
    PortalApp();

    void setup(AppManager* appManager, IApp* returnApp);

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

    enum class Mode {
        Idle,
        Running,
        Failed
    };

    Mode _mode = Mode::Idle;


    void requestFullRender();
    void requestPartialRender();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawContent(DisplayManager& display);

    AppManager* _appManager = nullptr;
    IApp* _returnApp = nullptr;

    bool _needsRender = true;
    RenderMode _renderMode = RenderMode::Full;
    bool _portalStartedOk = false;
};
