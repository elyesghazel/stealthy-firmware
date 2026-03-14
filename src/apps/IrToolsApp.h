#pragma once

#include "../core/IApp.h"

class AppManager;
class DisplayManager;
class ButtonEvent;
class MainMenuApp;
class IrManager;

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
        Captured
    };

    enum class RenderMode {
        Full,
        Partial
    };

    static constexpr int ITEM_COUNT = 3;
    static constexpr int VISIBLE_ITEMS = 3;

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

    int rowBaselineY(int visibleRow) const;

    AppManager* _appManager = nullptr;
    IApp* _returnApp = nullptr;

    Mode _mode = Mode::Menu;
    RenderMode _renderMode = RenderMode::Full;
    bool _needsRender = true;

    int _selectedIndex = 0;
    int _partialUpdateCount = 0;

    const char* _items[ITEM_COUNT] = {
        "Record",
        "Replay Last",
        "Back"
    };
};