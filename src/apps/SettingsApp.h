#pragma once
#include "core/IApp.h"
#include "DeviceSettings.h"

class AppManager;
class IApp;

enum class SettingsMode {
    Browse,
    Edit
};

enum class SettingsRenderMode {
    Full,
    Partial
};

class SettingsApp : public IApp {
public:
    SettingsApp();

    void setup(AppManager* appManager, IApp* returnApp, DeviceSettings* settings);

    void onEnter() override;
    void onExit() override;
    void handleButton(const ButtonEvent& event) override;
    void update() override;
    void render(DisplayManager& display) override;

private:
    static const int ITEM_COUNT = 5;

    AppManager* _appManager = nullptr;
    IApp* _returnApp = nullptr;
    DeviceSettings* _settings = nullptr;

    int _selectedIndex = 0;
    int _previousSelectedIndex = 0;
    int _partialUpdateCount = 0;

    bool _needsRender = true;
    SettingsMode _mode = SettingsMode::Browse;
    SettingsRenderMode _renderMode = SettingsRenderMode::Full;

    const char* _items[ITEM_COUNT] = {
        "Sleep Timeout",
        "Refresh Every",
        "Badge Status",
        "Start Screen",
        "Back"
    };

    void moveUp();
    void moveDown();
    void selectItem();
    void goBack();

    void requestFullRender();
    void requestPartialRender();

    void renderFull(DisplayManager& display);
    void renderPartial(DisplayManager& display);
    void drawSettingsList(DisplayManager& display);
    void drawValue(DisplayManager& display, int index, int y);

    int rowBaselineY(int index) const;
    int rowTopY(int index) const;

    const char* currentValueText(int index) const;
};