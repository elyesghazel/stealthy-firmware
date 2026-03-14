#include "PortalApp.h"
#include "../core/AppManager.h"
#include "../core/AppContext.h"
#include "../core/PortalManager.h"
#include "../core/LedManager.h"
#include "../drivers/DisplayManager.h"

namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;

    constexpr int TEXT_X = 10;
    constexpr int LINE1_Y = 62;
    constexpr int LINE2_Y = 80;
    constexpr int LINE3_Y = 98;
    constexpr int LINE4_Y = 116;
}

PortalApp::PortalApp() {
}

void PortalApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp = returnApp;
}

void PortalApp::onEnter() {
    _mode = Mode::Idle;
    _portalStartedOk = false;
    requestFullRender();
    Serial.println("[PortalApp] enter");
}


void PortalApp::onExit() {
    if (_appManager && _appManager->context() && _appManager->context()->portal) {
        _appManager->context()->portal->stop();
    }

    Serial.println("[PortalApp] exit");
}

void PortalApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) {
        return;
    }

    if (event.id == ButtonId::Back) {
        if (_appManager && _returnApp) {
            _appManager->switchTo(_returnApp);
        }
        return;
    }

    if (event.id == ButtonId::Select) {
        if (!_appManager || !_appManager->context() || !_appManager->context()->portal) {
            return;
        }

        if (_mode == Mode::Idle) {
            if (_appManager->context()->leds) {
                _appManager->context()->leds->setIdle();
            }
            // optional: prepare display before Wi-Fi startup
            delay(300);
            _portalStartedOk = _appManager->context()->portal->begin();
            _mode = _portalStartedOk ? Mode::Running : Mode::Failed;

            requestPartialRender();
        }
    }
}


void PortalApp::update() {
    if (_appManager && _appManager->context() && _appManager->context()->portal) {
        _appManager->context()->portal->update();
    }
}

void PortalApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    if (_renderMode == RenderMode::Full) {
        renderFull(display);
    } else {
        renderPartial(display);
    }

    _needsRender = false;
}

void PortalApp::requestFullRender() {
    _renderMode = RenderMode::Full;
    _needsRender = true;
}

void PortalApp::requestPartialRender() {
    _renderMode = RenderMode::Partial;
    _needsRender = true;
}

void PortalApp::drawContent(DisplayManager& display) {
    display.setTitleFont();
    display.setTextBlack();
    display.drawText(TITLE_X, TITLE_Y, "Portal");

    display.setDefaultFont();

    if (_mode == Mode::Idle) {
        display.drawText(TEXT_X, LINE1_Y, "Select = start portal");
        display.drawText(TEXT_X, LINE2_Y, "Back   = return");
        return;
    }

    if (_mode == Mode::Failed) {
        display.drawText(TEXT_X, LINE1_Y, "Portal start failed");
        display.drawText(TEXT_X, LINE2_Y, "Likely power dip");
        display.drawText(TEXT_X, LINE4_Y, "Back = return");
        return;
    }

    if (!_appManager || !_appManager->context() || !_appManager->context()->portal) {
        display.drawText(TEXT_X, LINE1_Y, "Portal unavailable");
        display.drawText(TEXT_X, LINE4_Y, "Back = return");
        return;
    }

    PortalManager* portal = _appManager->context()->portal;

    String ssid = String("SSID: ") + portal->ssid();
    String ip   = String("IP:   ") + portal->ipAddress();

    display.drawText(TEXT_X, LINE1_Y, "Portal active");
    display.drawText(TEXT_X, LINE2_Y, ssid);
    display.drawText(TEXT_X, LINE3_Y, ip);
    display.drawText(TEXT_X, LINE4_Y, "Back = stop portal");
}


void PortalApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        drawContent(display);
    } while (display.nextPage());

    Serial.println("[PortalApp] full render");
}

void PortalApp::renderPartial(DisplayManager& display) {
    int x = 0;
    int top = 24;
    int w = display.width();
    int h = display.height() - top;

    if (top % 2 != 0) top--;
    if (h % 2 != 0) h++;
    w = ((w + 7) / 8) * 8;

    display.startPartialWindowDraw(x, top, w, h);
    do {
        display.fillRectWhite(x, top, w, h);
        drawContent(display);
    } while (display.nextPage());

    Serial.println("[PortalApp] partial render");
}
