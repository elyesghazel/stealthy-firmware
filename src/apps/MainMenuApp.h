#pragma once
#include "core/IApp.h"
#include <WString.h>

class AppManager;
class IApp;

class MainMenuApp : public IApp {
public:
    MainMenuApp();

    void setup(AppManager* appManager, IApp* badgeApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    AppManager* _appManager = nullptr;
    IApp* _badgeApp = nullptr;

    static const int ITEM_COUNT = 4;
    String _items[ITEM_COUNT] = {
        "WiFi Tools",
        "BLE Tools",
        "IR Tools",
        "Settings"
    };

    int _selectedIndex = 0;
};