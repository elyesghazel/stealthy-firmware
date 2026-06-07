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

    // Profile switching
    int  _activeProfile = 0;

    // Tagline scrolling
    String        _tagline;
    int           _scrollOffset    = 0;
    unsigned long _lastScrollMs    = 0;
    unsigned long _scrollPauseUntil = 0;
    static constexpr int  VISIBLE_CHARS   = 25;
    static constexpr unsigned long SCROLL_INTERVAL_MS = 220;
    static constexpr unsigned long SCROLL_PAUSE_MS    = 1800;
};
