#pragma once
#include <Arduino.h>
#include "framework/IApp.h"

class AppManager;
class DisplayManager;

class AppleJuiceApp : public IApp {
public:
    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter()  override;
    void onExit()   override;
    void handleButton(const ButtonEvent& event) override;
    void update()   override;
    void render(DisplayManager& display) override;

private:
    void requestFullRender();
    void requestPartialRender();
    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawContent(DisplayManager& display);

    AppManager* _appManager = nullptr;
    IApp*       _returnApp  = nullptr;

    bool _needsRender = true;
    bool _fullRender  = true;

    uint32_t _lastRefresh = 0;
};
