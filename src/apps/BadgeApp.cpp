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

void BadgeApp::update() {}

void BadgeApp::render(DisplayManager& display) {
    display.clear();
    display.drawText(0, 10, "Stealthy");
    display.drawText(0, 30, "Name: Your Name");
    display.drawText(0, 50, "Status: Online");
    display.drawText(0, 80, "Press Select");
    display.updateFull();
}