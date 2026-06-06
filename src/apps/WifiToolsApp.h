#pragma once
#include "framework/IApp.h"

class AppManager;
class DisplayManager;

class WifiToolsApp : public IApp {
public:
    void setup(AppManager* appManager, IApp* returnApp, IApp* spammerApp, IApp* deauthApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    static constexpr int ITEM_COUNT   = 2;
    static constexpr int VISIBLE_ITEMS = 2;

    void requestFullRender();
    void requestPartialRender();
    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawMenu(DisplayManager& display);

    int rowBaselineY(int row) const;
    int rowTopY(int row) const;

    AppManager* _appManager  = nullptr;
    IApp*       _returnApp   = nullptr;
    IApp*       _spammerApp  = nullptr;
    IApp*       _deauthApp   = nullptr;

    const char* _items[ITEM_COUNT] = { "Beacon Flood", "WiFi Deauth" };

    int  _selectedIndex     = 0;
    int  _partialUpdates    = 0;
    bool _needsRender       = true;
    bool _fullRender        = true;
};
