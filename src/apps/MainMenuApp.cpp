#include "MainMenuApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

MainMenuApp::MainMenuApp() {}

void MainMenuApp::setup(AppManager* appManager, IApp* badgeApp) {
    _appManager = appManager;
    _badgeApp = badgeApp;
}

void MainMenuApp::onEnter() {
    Serial.println("[MainMenuApp] enter");
    _selectedIndex = 0;
}

void MainMenuApp::onExit() {
    Serial.println("[MainMenuApp] exit");
}

void MainMenuApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) {
        return;
    }

    switch (event.id) {
        case ButtonId::Up:
            if (_selectedIndex > 0) {
                _selectedIndex--;
            }
            break;

        case ButtonId::Down:
            if (_selectedIndex < ITEM_COUNT - 1) {
                _selectedIndex++;
            }
            break;

        case ButtonId::Back:
            if (_appManager && _badgeApp) {
                _appManager->switchTo(_badgeApp);
            }
            break;

        case ButtonId::Select:
            Serial.printf("[MainMenuApp] selected: %s\n", _items[_selectedIndex].c_str());
            break;
    }
}

void MainMenuApp::update() {}

void MainMenuApp::render(DisplayManager& display) {
    display.clear();
    display.drawText(0, 10, "Main Menu");
    display.drawMenu(_items, ITEM_COUNT, _selectedIndex);
    display.updatePartial();
}