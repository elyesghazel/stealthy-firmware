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

    void setup(
        AppManager* appManager,
        IApp* badgeApp,
        IApp* settingsApp,
        IApp* aboutApp
    );

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    static const int ITEM_COUNT = 5;
    static const int VISIBLE_ITEMS = 4;

    AppManager* _appManager = nullptr;
    IApp* _badgeApp = nullptr;
    IApp* _settingsApp = nullptr;
    IApp* _aboutApp = nullptr;

    const char* _items[ITEM_COUNT] = {
        "Badge",
        "Settings",
        "About",
        "WiFi Tools",
        "IR Tools"
    };

    int _selectedIndex = 0;
    int _scrollOffset = 0;
    int _partialUpdateCount = 0;

    bool _needsRender = true;
    MenuRenderMode _renderMode = MenuRenderMode::Full;

    void moveSelectionUp();
    void moveSelectionDown();
    void clampScrollToSelection();

    void requestFullRender();
    void requestPartialRender();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);

    void drawRow(DisplayManager& display, int visibleRow, int itemIndex, bool selected);
    void drawMenuItems(DisplayManager& display);
    void drawScrollIndicators(DisplayManager& display);

    int rowBaselineY(int visibleRow) const;
    int rowTopY(int visibleRow) const;
};