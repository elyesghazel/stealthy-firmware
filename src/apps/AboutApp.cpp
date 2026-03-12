#include "AboutApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"
#include "AppInfo.h"

AboutApp::AboutApp() {}

void AboutApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp = returnApp;
}

void AboutApp::onEnter() {
    Serial.println("[AboutApp] enter");
    _needsRender = true;
}

void AboutApp::onExit() {
    Serial.println("[AboutApp] exit");
}

void AboutApp::handleButton(const ButtonEvent& event) {
    if (!_appManager || !_returnApp) {
        return;
    }

    switch (event.id) {
        case ButtonId::Back:
        case ButtonId::Select:
            if (event.action == ButtonAction::Press) {
                _appManager->switchTo(_returnApp);
            }
            break;

        default:
            break;
    }
}

void AboutApp::update() {
}

void AboutApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(20, 38, AppInfo::PRODUCT_NAME);

        display.setDefaultFont();
        display.drawText(20, 62, "About");
        display.drawText(20, 80, AppInfo::FW_VERSION);
        display.drawText(20, 96, AppInfo::HW_REVISION);
        display.drawText(20, 112, AppInfo::DESCRIPTION);
        display.drawText(20, 128, AppInfo::WEBSITE);
    } while (display.nextPage());

    _needsRender = false;
}