#include "BadgeApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"
#include "../core/StorageManager.h"

BadgeApp::BadgeApp() {}

void BadgeApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp = returnApp;
}

void BadgeApp::onEnter() {
    Serial.println("[BadgeApp] enter");
    _needsRender = true;
}

void BadgeApp::onExit() {
    Serial.println("[BadgeApp] exit");
}

void BadgeApp::handleButton(const ButtonEvent& event) {
    if (!_appManager || !_returnApp) {
        return;
    }

    switch (event.id) {
        case ButtonId::Back:
            if (event.action == ButtonAction::Press) {
                _appManager->switchTo(_returnApp);
            }
            break;

        default:
            break;
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
        display.drawStatusBar();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(4, 32, "Badge App");
        String name = _appManager->context()->storage->getBadgeName();
        String status = _appManager->context()->storage->getBadgeStatus();

        display.setDefaultFont();
        display.setTextBlack();
        display.drawText(4, 80, ("Name: " + name).c_str());
        display.drawText(4, 110, ("Status: " + status).c_str());
        

        display.setDefaultFont();
        display.setTextBlack();
        display.drawText(50, 50, "Press BACK to return...");
    } while (display.nextPage());

    _needsRender = false;
}