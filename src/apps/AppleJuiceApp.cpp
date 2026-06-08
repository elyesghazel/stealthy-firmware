#include "AppleJuiceApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "display/DisplayManager.h"
#include "wifi/AppleJuice.h"

namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;
    constexpr int TEXT_X  = 10;
    constexpr int L1_Y    = 62;
    constexpr int L2_Y    = 78;
    constexpr int L3_Y    = 94;
    constexpr int L4_Y    = 110;

    static const char* MODE_NAMES[] = { "All devices", "Audio only", "Setup only" };
}

void AppleJuiceApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp  = returnApp;
}

void AppleJuiceApp::onEnter() {
    _lastRefresh = 0;
    requestFullRender();
    Serial.println("[AppleJuiceApp] enter");
}

void AppleJuiceApp::onExit() {
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    if (ctx && ctx->appleJuice && ctx->appleJuice->isRunning())
        ctx->appleJuice->stop();
    Serial.println("[AppleJuiceApp] exit");
}

void AppleJuiceApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) return;
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    auto* aj  = ctx ? ctx->appleJuice : nullptr;

    switch (event.id) {
        case ButtonId::Back:
            if (_appManager && _returnApp) _appManager->switchTo(_returnApp);
            break;

        case ButtonId::Select:
            if (!aj) break;
            if (aj->isRunning()) {
                aj->stop();
            } else {
                aj->begin(aj->mode());
            }
            requestPartialRender();
            break;

        case ButtonId::Up:
        case ButtonId::Down:
            if (!aj || aj->isRunning()) break;
            {
                int m = (int)aj->mode();
                if (event.id == ButtonId::Up)
                    m = (m == 0) ? 2 : m - 1;
                else
                    m = (m == 2) ? 0 : m + 1;
                aj->setMode((AppleMode)m);
                requestPartialRender();
            }
            break;

        default: break;
    }
}

void AppleJuiceApp::update() {
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    auto* aj  = ctx ? ctx->appleJuice : nullptr;
    if (!aj || !aj->isRunning()) return;

    aj->update();

    // Refresh display every second while running
    if (millis() - _lastRefresh >= 1000) {
        _lastRefresh = millis();
        requestPartialRender();
    }
}

void AppleJuiceApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    if (_fullRender) renderFull(display);
    else             renderPartial(display);
    _needsRender = false;
}

void AppleJuiceApp::requestFullRender()    { _fullRender = true;  _needsRender = true; }
void AppleJuiceApp::requestPartialRender() { _fullRender = false; _needsRender = true; }

void AppleJuiceApp::drawContent(DisplayManager& display) {
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    auto* aj  = ctx ? ctx->appleJuice : nullptr;

    display.setTitleFont();
    display.setTextBlack();
    display.drawText(TITLE_X, TITLE_Y, "Apple Juice");
    display.setDefaultFont();

    if (!aj) {
        display.drawText(TEXT_X, L1_Y, "Not available");
        return;
    }

    bool running = aj->isRunning();
    int  m       = (int)aj->mode();

    // Line 1: mode
    String modeLine = String("Mode: ") + MODE_NAMES[m];
    display.drawText(TEXT_X, L1_Y, modeLine.c_str());

    if (running) {
        // Line 2: current device
        String devLine = String("Dev: ") + aj->currentName();
        display.drawText(TEXT_X, L2_Y, devLine.c_str());

        // Line 3: sent count
        String cntLine = String("Sent: ") + aj->sentCount();
        display.drawText(TEXT_X, L3_Y, cntLine.c_str());

        // Line 4: status + hint
        display.drawText(TEXT_X, L4_Y, "Select=stop  Back=exit");
    } else {
        display.drawText(TEXT_X, L2_Y, "Up/Dn = change mode");
        display.drawText(TEXT_X, L3_Y, "Select = start");
        display.drawText(TEXT_X, L4_Y, "Back   = exit");
    }
}

void AppleJuiceApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        drawContent(display);
    } while (display.nextPage());
}

void AppleJuiceApp::renderPartial(DisplayManager& display) {
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
