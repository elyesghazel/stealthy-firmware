#pragma once
#include <Arduino.h>
#include "framework/IApp.h"

class AppManager;
class IApp;
class DisplayManager;

class BadgeApp : public IApp {
public:
    BadgeApp();
    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter() override;
    void onExit()  override;
    void handleButton(const ButtonEvent& event) override;
    void update()  override;
    void render(DisplayManager& display) override;

private:
    static void drawLockBitmap(DisplayManager& display, int cx, int cy);

    AppManager* _appManager  = nullptr;
    IApp*       _returnApp   = nullptr;
    bool        _needsRender = true;
    int         _irCount     = 0;
    int         _ssidCount   = 0;
};
