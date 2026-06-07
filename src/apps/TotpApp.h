#pragma once
#include "framework/IApp.h"
#include <Arduino.h>

class AppManager;
class DisplayManager;
class TotpManager;

enum class TotpRenderMode { Full, Partial };

class TotpApp : public IApp {
public:
    TotpApp();
    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter()  override;
    void onExit()   override;
    void handleButton(const ButtonEvent& event) override;
    void update()   override;
    void render(DisplayManager& display) override;

private:
    AppManager*  _appManager = nullptr;
    IApp*        _returnApp  = nullptr;
    TotpManager* _totp       = nullptr;

    int  _selectedIndex  = 0;
    int  _accountCount   = 0;
    bool _needsRender    = true;
    TotpRenderMode _renderMode = TotpRenderMode::Full;

    int  _lastSecondsRemaining = -1;
    unsigned long _lastUpdateMs = 0;
    int  _partialCount = 0;

    void requestFullRender();
    void requestPartialRender();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawContent(DisplayManager& display);

    void drawDigitBoxes(DisplayManager& display, const String& code);
    void drawProgressBar(DisplayManager& display);
    void drawNoAccounts(DisplayManager& display);
    void drawNoTimeSync(DisplayManager& display);
};
