#include "StartScreenApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

StartScreenApp::StartScreenApp() {}

void StartScreenApp::setup(AppManager* appManager, IApp* nextApp) {
    _appManager = appManager;
    _nextApp = nextApp;
}

void StartScreenApp::onEnter() {
    Serial.println("[StartScreenApp] enter");
    _needsRender = true;
}

void StartScreenApp::onExit() {
    Serial.println("[StartScreenApp] exit");
}

void StartScreenApp::handleButton(const ButtonEvent& event) {
    if (!_appManager || !_nextApp) {
        return;
    }

    switch (event.id) {
        case ButtonId::Select:
            if (event.action == ButtonAction::Press) {
                _appManager->switchTo(_nextApp);
            }
            break;

        default:
            break;
    }
}

void StartScreenApp::update() {
}

void StartScreenApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    display.startFullWindowDraw();
    do {
        display.fillWhite();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(10, 30, "Welcome to Stealthy");

        display.setDefaultFont();
        display.setTextBlack();
        display.drawText(50, 50, "Press SEL to start...");
    } while (display.nextPage());

    _needsRender = false;
}