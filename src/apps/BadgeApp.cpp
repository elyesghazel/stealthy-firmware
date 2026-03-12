#include "BadgeApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

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

        display.setDefaultFont();
        display.setTextBlack();
        display.drawText(50, 50, "Press BACK to return...");
    } while (display.nextPage());

    _needsRender = false;
}