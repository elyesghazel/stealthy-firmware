#pragma once
#include <Arduino.h>
#include "framework/IApp.h"

class AppManager;
class DisplayManager;

class WifiDeauthApp : public IApp {
public:
    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    enum class State { Scanning, SelectTarget, Attacking };

    static constexpr int VISIBLE_ITEMS = 4;
    static constexpr int ROW_SPACING   = 16;
    static constexpr int FIRST_ROW_Y   = 56;

    void requestFullRender();
    void requestPartialRender();
    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawContent(DisplayManager& display);
    void drawApList(DisplayManager& display);

    void clampScroll();
    int  rowBaselineY(int visibleRow) const;

    AppManager* _appManager = nullptr;
    IApp*       _returnApp  = nullptr;

    State    _state        = State::Scanning;
    int      _scanFrames   = 0;
    int      _selectedIdx  = 0;
    int      _scrollOffset = 0;
    uint32_t _attackStartMs  = 0;
    uint32_t _lastRenderMs   = 0;

    bool _needsRender = true;
    bool _fullRender  = true;
};
