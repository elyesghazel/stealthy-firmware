#pragma once
#include "core/IApp.h"

class AppManager;
class IApp;

enum class MenuRenderMode {
    Full,
    Partial
};

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
    const char* _items[ITEM_COUNT] = {
        "WiFi Tools",
        "BLE Tools",
        "IR Tools",
        "Settings"
    };

    int _selectedIndex = 0;
    int _previousSelectedIndex = 0;
    bool _needsRender = true;
    int _partialUpdateCount = 0;
    MenuRenderMode _renderMode = MenuRenderMode::Full;
};