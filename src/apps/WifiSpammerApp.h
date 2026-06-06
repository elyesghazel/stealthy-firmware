#pragma once
#include "framework/IApp.h"

class AppManager;
class DisplayManager;

class WifiSpammerApp : public IApp {
public:
    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    enum class Mode { Idle, Running, NoSSIDs };

    void requestFullRender();
    void requestPartialRender();
    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawContent(DisplayManager& display);

    AppManager* _appManager = nullptr;
    IApp*       _returnApp  = nullptr;

    Mode _mode        = Mode::Idle;
    bool _needsRender = true;
    bool _fullRender  = true;
};
