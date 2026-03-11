#include "BadgeApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

BadgeApp::BadgeApp() {}

void BadgeApp::setup(AppManager* appManager, IApp* mainMenuApp) {
    _appManager = appManager;
    _mainMenuApp = mainMenuApp;
}

void BadgeApp::onEnter() {
    Serial.println("[BadgeApp] enter");
    _needsRender = true;
}

void BadgeApp::onExit() {
    Serial.println("[BadgeApp] exit");
}

void BadgeApp::handleButton(const ButtonEvent& event) {
    if (_appManager && _mainMenuApp &&
        event.id == ButtonId::Select &&
        event.action == ButtonAction::Press) {
        _appManager->switchTo(_mainMenuApp);
    }
}

void BadgeApp::update() {
}

void BadgeApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    display.startFullWindowDraw();
    do {
        display.fillWhite();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(10, 25, "Stealthy");

        display.setDefaultFont();
        display.drawText(10, 55, "Name:");
        display.drawText(60, 55, _name);

        display.drawText(10, 75, "Status:");
        display.drawText(60, 75, _status);

        display.drawText(10, 105, "Press Select");
    } while (display.nextPage());

    _needsRender = false;
}