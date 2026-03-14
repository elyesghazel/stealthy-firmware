#include "StartScreenApp.h"
#include "../core/AppManager.h"
#include "../core/AppContext.h"
#include "../core/LedManager.h"
#include "../drivers/DisplayManager.h"

namespace {
    constexpr int TITLE_X = 6;
    constexpr int TITLE_Y = 22;

    constexpr int TEXT_X = 8;
    constexpr int LINE1_Y = 42;
    constexpr int LINE2_Y = 56;
    constexpr int LINE3_Y = 70;
    constexpr int LINE4_Y = 84;
    constexpr int LINE5_Y = 98;

    constexpr int BAR_X = 8;
    constexpr int BAR_Y = 112;
    constexpr int BAR_W = 112;
    constexpr int BAR_H = 10;

    constexpr unsigned long STAGE_DELAY_MS = 350;
    constexpr int FINAL_STAGE = 5;
}

StartScreenApp::StartScreenApp() {
}

void StartScreenApp::setup(AppManager* appManager, IApp* nextApp) {
    _appManager = appManager;
    _nextApp = nextApp;
}

void StartScreenApp::onEnter() {
    _stage = 0;
    _stageStartedMs = millis();
    requestFullRender();

    if (_appManager && _appManager->context() && _appManager->context()->leds) {
        _appManager->context()->leds->showBooting();
    }

    Serial.println("[StartScreenApp] enter");
}

void StartScreenApp::onExit() {
    Serial.println("[StartScreenApp] exit");
}

void StartScreenApp::handleButton(const ButtonEvent& event) {
    (void)event;
}

void StartScreenApp::update() {
    unsigned long now = millis();

    if (_stage < FINAL_STAGE && (now - _stageStartedMs) >= STAGE_DELAY_MS) {
        _stage++;
        _stageStartedMs = now;
        requestPartialRender();
    }

    if (_stage >= FINAL_STAGE && (now - _stageStartedMs) >= 350) {
        if (_appManager && _nextApp) {
            if (_appManager->context() && _appManager->context()->leds) {
                _appManager->context()->leds->showSuccess();
            }
            _appManager->switchTo(_nextApp);
        }
    }
}

void StartScreenApp::render(DisplayManager& display) {
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

void StartScreenApp::requestFullRender() {
    _renderMode = RenderMode::Full;
    _needsRender = true;
}

void StartScreenApp::requestPartialRender() {
    _renderMode = RenderMode::Partial;
    _needsRender = true;
}

void StartScreenApp::drawProgressBar(DisplayManager& display, int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    display.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H);

    int innerW = ((BAR_W - 2) * percent) / 100;
    if (innerW > 0) {
        display.fillRect(BAR_X + 1, BAR_Y + 1, innerW, BAR_H - 2);
    }
}

void StartScreenApp::drawBootContent(DisplayManager& display) {
    display.setTextBlack();

    display.setTitleFont();
    display.drawText(TITLE_X, TITLE_Y, "root@stealthy:~# boot");

    display.setDefaultFont();

    if (_stage >= 1) {
        display.drawText(TEXT_X, LINE1_Y, "[ ok ] power rails");
    }
    if (_stage >= 2) {
        display.drawText(TEXT_X, LINE2_Y, "[ ok ] eink driver");
    }
    if (_stage >= 3) {
        display.drawText(TEXT_X, LINE3_Y, "[ ok ] littlefs mount");
    }
    if (_stage >= 4) {
        display.drawText(TEXT_X, LINE4_Y, "[ ok ] ir subsystem");
    }
    if (_stage >= 5) {
        display.drawText(TEXT_X, LINE5_Y, "[ ok ] ui online");
    }

    int percent = 0;
    switch (_stage) {
        case 0: percent = 5; break;
        case 1: percent = 20; break;
        case 2: percent = 40; break;
        case 3: percent = 65; break;
        case 4: percent = 85; break;
        case 5: percent = 100; break;
    }

    drawProgressBar(display, percent);
}

void StartScreenApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        drawBootContent(display);
    } while (display.nextPage());

    Serial.printf("[StartScreenApp] full render stage %d\n", _stage);
}

void StartScreenApp::renderPartial(DisplayManager& display) {
    int x = 0;
    int top = 28;
    int w = display.width();
    int h = (BAR_Y + BAR_H + 4) - top;

    if (top < 0) top = 0;
    if (top % 2 != 0) top--;
    if (h % 2 != 0) h++;
    w = ((w + 7) / 8) * 8;

    display.startPartialWindowDraw(x, top, w, h);
    do {
        display.fillRectWhite(x, top, w, h);
        drawBootContent(display);
    } while (display.nextPage());

    Serial.printf("[StartScreenApp] partial render stage %d\n", _stage);
}