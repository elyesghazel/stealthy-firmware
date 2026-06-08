#include "WifiSpammerApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "display/DisplayManager.h"
#include "wifi/WifiSpammer.h"
#include "portal/PortalManager.h"
#include "wifi/TrollPortal.h"
#include "storage/StorageManager.h"
#include "led/LedManager.h"

namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;
    constexpr int TEXT_X  = 10;
    constexpr int L1_Y    = 62;
    constexpr int L2_Y    = 78;
    constexpr int L3_Y    = 94;
    constexpr int L4_Y    = 110;
}

void WifiSpammerApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp  = returnApp;
}

void WifiSpammerApp::onEnter() {
    auto* ctx = _appManager ? _appManager->context() : nullptr;

    _portalWasRunning = false;

    if (ctx && ctx->storage && ctx->spammer) {
        auto ssids = ctx->storage->loadSpamSSIDs();
        ctx->spammer->setSSIDs(ssids);
        _mode = ssids.empty() ? Mode::NoSSIDs : Mode::Idle;
    } else {
        _mode = Mode::Idle;
    }

    requestFullRender();
    Serial.println("[WifiSpammerApp] enter");
}

void WifiSpammerApp::onExit() {
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    if (ctx && ctx->spammer && ctx->spammer->isRunning()) {
        ctx->spammer->stop();
        if (ctx->troll) ctx->troll->stop();
        if (_portalWasRunning && ctx->portal) ctx->portal->begin();
        _portalWasRunning = false;
    }
    if (ctx && ctx->leds) {
        ctx->leds->setIdle();
    }
    Serial.println("[WifiSpammerApp] exit");
}

void WifiSpammerApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) return;

    auto* ctx = _appManager ? _appManager->context() : nullptr;

    if (event.id == ButtonId::Back) {
        if (_appManager && _returnApp) _appManager->switchTo(_returnApp);
        return;
    }

    if (event.id == ButtonId::Select) {
        if (_mode == Mode::NoSSIDs) return;

        if (_mode == Mode::Idle) {
            // Stop main portal to free WiFi
            if (ctx && ctx->portal && ctx->portal->isRunning()) {
                ctx->portal->stop();
                _portalWasRunning = true;
            }
            // Start troll captive portal — karma matches any flood SSID
            if (ctx && ctx->troll && ctx->storage) {
                auto ssids = ctx->storage->loadSpamSSIDs();
                ctx->troll->setSSIDs(ssids);
                // Initial AP SSID: explicit troll SSID, or first flood SSID
                String trollSsid = ctx->storage->getTrollSsid();
                if (trollSsid.isEmpty() && !ssids.empty()) trollSsid = ssids[0];
                if (!trollSsid.isEmpty()) {
                    ctx->troll->begin(trollSsid);
                }
            }
            // Start spammer — detects AP_STA from troll portal, injects on STA
            if (ctx && ctx->spammer && ctx->spammer->begin()) {
                _mode = Mode::Running;
                if (ctx->leds) ctx->leds->showIrTransmit();
                requestPartialRender();
            } else {
                if (ctx && ctx->troll) ctx->troll->stop();
                if (_portalWasRunning && ctx && ctx->portal) ctx->portal->begin();
                _portalWasRunning = false;
            }
        } else if (_mode == Mode::Running) {
            if (ctx && ctx->spammer) ctx->spammer->stop();
            if (ctx && ctx->troll)   ctx->troll->stop();
            if (ctx && ctx->leds)    ctx->leds->setIdle();
            if (_portalWasRunning && ctx && ctx->portal) ctx->portal->begin();
            _portalWasRunning = false;
            _mode = Mode::Idle;
            requestPartialRender();
        }
    }
}

void WifiSpammerApp::update() {}

void WifiSpammerApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    if (_fullRender) renderFull(display);
    else             renderPartial(display);
    _needsRender = false;
}

void WifiSpammerApp::requestFullRender() {
    _fullRender = true; _needsRender = true;
}

void WifiSpammerApp::requestPartialRender() {
    _fullRender = false; _needsRender = true;
}

void WifiSpammerApp::drawContent(DisplayManager& display) {
    auto* ctx = _appManager ? _appManager->context() : nullptr;

    display.setTitleFont();
    display.setTextBlack();
    display.drawText(TITLE_X, TITLE_Y, "Beacon Flood");
    display.setDefaultFont();

    if (_mode == Mode::NoSSIDs) {
        display.drawText(TEXT_X, L1_Y, "No SSIDs configured.");
        display.drawText(TEXT_X, L2_Y, "Add them in the");
        display.drawText(TEXT_X, L3_Y, "portal WiFi tab.");
        display.drawText(TEXT_X, L4_Y, "Back = exit");
        return;
    }

    int count = (ctx && ctx->spammer) ? ctx->spammer->ssidCount() : 0;
    String countLine = "SSIDs: " + String(count);
    display.drawText(TEXT_X, L1_Y, countLine.c_str());

    if (_mode == Mode::Idle) {
        display.drawText(TEXT_X, L2_Y, "Status: OFF");
        display.drawText(TEXT_X, L3_Y, "Select = start flood");
        display.drawText(TEXT_X, L4_Y, "Back   = exit");
    } else {
        display.drawText(TEXT_X, L2_Y, "Status: FLOODING");
        display.drawText(TEXT_X, L3_Y, _portalWasRunning ? "Portal paused" : "Select = stop");
        display.drawText(TEXT_X, L4_Y, "Select=stop  Back=exit");
    }
}

void WifiSpammerApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        drawContent(display);
    } while (display.nextPage());
}

void WifiSpammerApp::renderPartial(DisplayManager& display) {
    int x   = 0;
    int top = 24;
    int w   = display.width();
    int h   = display.height() - top;

    if (top % 2 != 0) top--;
    if (h   % 2 != 0) h++;
    w = ((w + 7) / 8) * 8;

    display.startPartialWindowDraw(x, top, w, h);
    do {
        display.fillRectWhite(x, top, w, h);
        drawContent(display);
    } while (display.nextPage());
}
