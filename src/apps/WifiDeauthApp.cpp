#include "WifiDeauthApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "wifi/WifiKarma.h"
#include "display/DisplayManager.h"
#include "wifi/WifiDeauth.h"
#include "wifi/WifiSpammer.h"
#include "portal/PortalManager.h"
#include "led/LedManager.h"

namespace {
    constexpr int TITLE_X  = 5;
    constexpr int TITLE_Y  = 38;
    constexpr int TEXT_X   = 10;
    constexpr int CARET_X  = 10;
    constexpr int LABEL_X  = 24;

    constexpr int L1_Y = 62;
    constexpr int L2_Y = 78;
    constexpr int L3_Y = 94;
    constexpr int L4_Y = 110;

    constexpr int SCROLL_TOP_Y = 52;
    constexpr int SCROLL_BOT_Y = 112;
}

void WifiDeauthApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp  = returnApp;
}

void WifiDeauthApp::onEnter() {
    auto* ctx = _appManager ? _appManager->context() : nullptr;

    // Stop spammer if running (mutually exclusive with deauth)
    if (ctx && ctx->spammer && ctx->spammer->isRunning())
        ctx->spammer->stop();
    // Stop karma sniff if running (shares promiscuous mode with deauth)
    if (ctx && ctx->karma && ctx->karma->isSniffing())
        ctx->karma->stopSniff();

    _state       = State::Scanning;
    _scanFrames  = 0;
    _selectedIdx = 0;
    _scrollOffset = 0;
    requestFullRender();
    Serial.println("[WifiDeauthApp] enter");
}

void WifiDeauthApp::onExit() {
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    if (ctx && ctx->deauth && ctx->deauth->isRunning()) {
        ctx->deauth->stop();
    }
    if (ctx && ctx->leds) {
        ctx->leds->setIdle();
    }
    Serial.println("[WifiDeauthApp] exit");
}

void WifiDeauthApp::handleButton(const ButtonEvent& event) {
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    const bool moveEvent =
        (event.action == ButtonAction::Press || event.action == ButtonAction::Repeat);

    if (event.id == ButtonId::Back && event.action == ButtonAction::Press) {
        if (_state == State::Attacking) {
            if (ctx && ctx->deauth) ctx->deauth->stop();
            if (ctx && ctx->leds)   ctx->leds->setIdle();
            _state = State::SelectTarget;
            requestFullRender();
        } else {
            if (_appManager && _returnApp) _appManager->switchTo(_returnApp);
        }
        return;
    }

    if (_state != State::SelectTarget) return;

    auto* deauth = (ctx && ctx->deauth) ? ctx->deauth : nullptr;
    if (!deauth) return;

    const auto& aps = deauth->getApList();

    if (event.id == ButtonId::Up && moveEvent && _selectedIdx > 0) {
        _selectedIdx--;
        clampScroll();
        requestPartialRender();
    }
    if (event.id == ButtonId::Down && moveEvent && _selectedIdx < (int)aps.size() - 1) {
        _selectedIdx++;
        clampScroll();
        requestPartialRender();
    }
    if (event.id == ButtonId::Select && event.action == ButtonAction::Press) {
        if (deauth->startDeauth(_selectedIdx)) {
            _state          = State::Attacking;
            _attackStartMs  = millis();
            _lastRenderMs   = 0;
            if (ctx && ctx->leds) ctx->leds->showIrTransmit();
            requestFullRender();
        }
    }
}

void WifiDeauthApp::update() {
    if (_state == State::Scanning) {
        _scanFrames++;
        if (_scanFrames < 3) return; // wait for "Scanning..." to render first

        auto* ctx    = _appManager ? _appManager->context() : nullptr;
        auto* deauth = (ctx && ctx->deauth) ? ctx->deauth : nullptr;

        if (deauth) deauth->scan(); // blocking ~3-5s

        _state       = State::SelectTarget;
        _selectedIdx = 0;
        _scrollOffset = 0;
        requestFullRender();
        return;
    }

    if (_state == State::Attacking) {
        // Partial re-render every 2 s to show live frame / EAPOL counts
        uint32_t now = millis();
        if (now - _lastRenderMs >= 2000) {
            _lastRenderMs = now;
            requestPartialRender();
        }
    }
}

void WifiDeauthApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    if (_fullRender) renderFull(display);
    else             renderPartial(display);
    _needsRender = false;
}

void WifiDeauthApp::requestFullRender() {
    _fullRender = true; _needsRender = true;
}

void WifiDeauthApp::requestPartialRender() {
    _fullRender = false; _needsRender = true;
}

void WifiDeauthApp::clampScroll() {
    if (_selectedIdx < _scrollOffset) _scrollOffset = _selectedIdx;
    if (_selectedIdx >= _scrollOffset + VISIBLE_ITEMS)
        _scrollOffset = _selectedIdx - VISIBLE_ITEMS + 1;
    if (_scrollOffset < 0) _scrollOffset = 0;
}

int WifiDeauthApp::rowBaselineY(int visibleRow) const {
    return FIRST_ROW_Y + visibleRow * ROW_SPACING;
}

void WifiDeauthApp::drawApList(DisplayManager& display) {
    auto* ctx    = _appManager ? _appManager->context() : nullptr;
    auto* deauth = (ctx && ctx->deauth) ? ctx->deauth : nullptr;
    if (!deauth) return;

    const auto& aps = deauth->getApList();

    display.setDefaultFont();
    display.setTextBlack();

    if (aps.empty()) {
        display.drawText(TEXT_X, L1_Y, "No networks found.");
        display.drawText(TEXT_X, L2_Y, "Back = exit");
        return;
    }

    for (int v = 0; v < VISIBLE_ITEMS; v++) {
        int idx = _scrollOffset + v;
        if (idx >= (int)aps.size()) break;

        int y = rowBaselineY(v);
        display.drawText(CARET_X, y, idx == _selectedIdx ? ">" : " ");

        // Truncate SSID to 14 chars, then append signal
        String ssid = aps[idx].ssid;
        if (ssid.isEmpty()) ssid = "(hidden)";
        if (ssid.length() > 14) ssid = ssid.substring(0, 14);

        String sig = WifiDeauth::rssiLabel(aps[idx].rssi);
        String line = ssid + " " + sig;
        display.drawText(LABEL_X, y, line.c_str());
    }

    // Scroll indicators
    if (_scrollOffset > 0)
        display.drawText(2, SCROLL_TOP_Y, "^");
    if (_scrollOffset + VISIBLE_ITEMS < (int)aps.size())
        display.drawText(2, SCROLL_BOT_Y, "v");
}

void WifiDeauthApp::drawContent(DisplayManager& display) {
    display.setTitleFont();
    display.setTextBlack();
    display.drawText(TITLE_X, TITLE_Y, "WiFi Deauth");
    display.setDefaultFont();

    if (_state == State::Scanning) {
        display.drawText(TEXT_X, L1_Y, "Scanning...");
        display.drawText(TEXT_X, L2_Y, "Please wait");
        return;
    }

    if (_state == State::SelectTarget) {
        auto* ctx    = _appManager ? _appManager->context() : nullptr;
        auto* deauth = (ctx && ctx->deauth) ? ctx->deauth : nullptr;
        int n = deauth ? (int)deauth->getApList().size() : 0;

        if (n == 0) {
            display.drawText(TEXT_X, L1_Y, "No APs found.");
            display.drawText(TEXT_X, L3_Y, "Back = exit");
        } else {
            drawApList(display);
        }
        return;
    }

    if (_state == State::Attacking) {
        auto* ctx    = _appManager ? _appManager->context() : nullptr;
        auto* deauth = (ctx && ctx->deauth) ? ctx->deauth : nullptr;

        String target = deauth ? deauth->getTargetSsid() : "?";
        if (target.isEmpty()) target = "(hidden)";
        if (target.length() > 21) target = target.substring(0, 21);

        int frames = deauth ? deauth->getFramesSent() : 0;
        int eapol  = deauth ? deauth->getEapolCount()  : 0;

        uint32_t elapsed = (millis() - _attackStartMs) / 1000;
        char elapsedBuf[10];
        if (elapsed >= 60) snprintf(elapsedBuf, sizeof(elapsedBuf), "%dm%ds", (int)(elapsed/60), (int)(elapsed%60));
        else                snprintf(elapsedBuf, sizeof(elapsedBuf), "%ds",    (int)elapsed);

        char framesBuf[22];
        char eapolBuf[22];
        snprintf(framesBuf, sizeof(framesBuf), "Frames: %d", frames);
        snprintf(eapolBuf,  sizeof(eapolBuf),  "EAPOL:  %d", eapol);

        char bottomBuf[28];
        snprintf(bottomBuf, sizeof(bottomBuf), "%s   Back=stop", elapsedBuf);

        display.drawText(TEXT_X, L1_Y, target.c_str());
        display.drawText(TEXT_X, L2_Y, framesBuf);
        display.drawText(TEXT_X, L3_Y, eapolBuf);
        display.drawText(TEXT_X, L4_Y, bottomBuf);
    }
}

void WifiDeauthApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        drawContent(display);
    } while (display.nextPage());
}

void WifiDeauthApp::renderPartial(DisplayManager& display) {
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
