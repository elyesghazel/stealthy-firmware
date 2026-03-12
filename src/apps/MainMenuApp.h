#pragma once
#include "core/IApp.h"

class AppManager;
class IApp;
class DisplayManager;

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
    static const int ITEM_COUNT = 4;

    AppManager* _appManager = nullptr;
    IApp* _badgeApp = nullptr;

    const char* _items[ITEM_COUNT] = {
        "WiFi Tools",
        "BLE Tools",
        "IR Tools",
        "Settings"
    };

    int _selectedIndex = 0;
    int _previousSelectedIndex = 0;
    int _partialUpdateCount = 0;

    bool _needsRender = true;
    MenuRenderMode _renderMode = MenuRenderMode::Full;

    void moveSelectionUp();
    void moveSelectionDown();

    void requestFullRender();
    void requestPartialRender();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);

    void drawRow(DisplayManager& display, int index, bool selected);
    int rowBaselineY(int index) const;
    int rowTopY(int index) const;
};