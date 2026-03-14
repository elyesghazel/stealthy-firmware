#pragma once

#include "../core/IApp.h"
#include <vector>
#include <Arduino.h>
#include "../core/StorageManager.h"

class AppManager;
class DisplayManager;
class ButtonEvent;


class IrToolsApp : public IApp {
public:
    IrToolsApp();

    void setup(AppManager* appManager, IApp* returnApp);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    enum class Mode {
        Menu,
        Recording,
        Captured,
        SavedList
    };

    enum class RenderMode {
        Full,
        Partial
    };

    static constexpr int MENU_ITEM_COUNT = 5;
    static constexpr int VISIBLE_ITEMS = 4;

    void requestFullRender();
    void requestPartialRender();

    void moveUp();
    void moveDown();
    void selectItem();
    void goBack();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);

    void drawMenu(DisplayManager& display);
    void drawCapturedInfo(DisplayManager& display);
    void drawSavedList(DisplayManager& display);
    void saveLastCapture();

    int rowBaselineY(int visibleRow) const;
    void refreshSavedItems();
    void clampSavedScroll();

    AppManager* _appManager = nullptr;
    IApp* _returnApp = nullptr;

    Mode _mode = Mode::Menu;
    RenderMode _renderMode = RenderMode::Full;
    bool _needsRender = true;

    int _selectedIndex = 0;
    int _partialUpdateCount = 0;

    int _savedSelectedIndex = 0;
    int _savedScrollOffset = 0;

    
    std::vector<IrSavedItem> _savedItems;

    unsigned long _saveFeedbackUntilMs = 0;
    bool _showSavedMessage = false;
    const char* _menuItems[MENU_ITEM_COUNT] = {
        "Record",
        "Replay Last",
        "Save Last",
        "Saved",
        "Back"
    };
};