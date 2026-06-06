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

    void onEnter()  override;
    void onExit()   override;
    void handleButton(const ButtonEvent& event) override;
    void update()   override;
    void render(DisplayManager& display) override;

    static constexpr int MODE_COUNT = 3;  // exposed for free helper in .cpp

private:
    enum class Mode : uint8_t { Text = 0, QrSide = 1, QrFull = 2, COUNT = 3 };

    void renderText   (DisplayManager& display);
    void renderQrSide (DisplayManager& display);
    void renderQrFull (DisplayManager& display);

    AppManager* _appManager  = nullptr;
    IApp*       _returnApp   = nullptr;
    bool        _needsRender = true;
    Mode        _mode        = Mode::Text;
};
