#include "IrToolsApp.h"
#include "../core/AppManager.h"
#include "../core/AppContext.h"
#include "../core/IrManager.h"
#include "../drivers/DisplayManager.h"
#include "../drivers/ButtonManager.h"
#include "../drivers/IrDriver.h"
#include <cstdio> // for snprintf


namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;

    constexpr int CARET_X = 12;
    constexpr int LABEL_X = 28;

    constexpr int FIRST_ROW_Y = 66;
    constexpr int ROW_SPACING = 18;

    constexpr int INFO_X = 10;
    constexpr int INFO_Y1 = 66;
    constexpr int INFO_Y2 = 84;
    constexpr int INFO_Y3 = 102;
    constexpr int INFO_Y4 = 120;
}

IrToolsApp::IrToolsApp() {
}

void IrToolsApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp = returnApp;
}

void IrToolsApp::onEnter() {
    Serial.println("[IrToolsApp] enter");
    _mode = Mode::Menu;
    _selectedIndex = 0;
    _partialUpdateCount = 0;
    requestFullRender();
}

void IrToolsApp::onExit() {
    Serial.println("[IrToolsApp] exit");
}

void IrToolsApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) {
        return;
    }

    switch (event.id) {
        case ButtonId::Up:
            moveUp();
            break;

        case ButtonId::Down:
            moveDown();
            break;

        case ButtonId::Select:
            selectItem();
            break;

        case ButtonId::Back:
            goBack();
            break;
    }
}

void IrToolsApp::update() {
    if (!_appManager || !_appManager->context() || !_appManager->context()->ir) {
        return;
    }

    IrManager* ir = _appManager->context()->ir;
    ir->update();

    if (_mode == Mode::Recording && ir->hasNewCapture()) {
        ir->clearNewCaptureFlag();
        _mode = Mode::Captured;
        requestFullRender();
    }
}

void IrToolsApp::render(DisplayManager& display) {
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

void IrToolsApp::requestFullRender() {
    _renderMode = RenderMode::Full;
    _needsRender = true;
}

void IrToolsApp::requestPartialRender() {
    _renderMode = RenderMode::Partial;
    _needsRender = true;
}

int IrToolsApp::rowBaselineY(int visibleRow) const {
    return FIRST_ROW_Y + visibleRow * ROW_SPACING;
}

void IrToolsApp::moveUp() {
    if (_mode != Mode::Menu) {
        return;
    }

    if (_selectedIndex > 0) {
        _selectedIndex--;
    } else {
        _selectedIndex = ITEM_COUNT - 1;
    }

    _partialUpdateCount++;
    if (_partialUpdateCount >= 20) {
        _partialUpdateCount = 0;
        requestFullRender();
    } else {
        requestPartialRender();
    }
}

void IrToolsApp::moveDown() {
    if (_mode != Mode::Menu) {
        return;
    }

    if (_selectedIndex < ITEM_COUNT - 1) {
        _selectedIndex++;
    } else {
        _selectedIndex = 0;
    }

    _partialUpdateCount++;
    if (_partialUpdateCount >= 20) {
        _partialUpdateCount = 0;
        requestFullRender();
    } else {
        requestPartialRender();
    }
}

void IrToolsApp::selectItem() {
    if (!_appManager || !_appManager->context() || !_appManager->context()->ir) {
        return;
    }

    IrManager* ir = _appManager->context()->ir;

    if (_mode == Mode::Recording) {
        return;
    }

    if (_mode == Mode::Captured) {
        ir->replayLast();
        requestPartialRender();
        return;
    }

    switch (_selectedIndex) {
        case 0:
            ir->startRecording();
            _mode = Mode::Recording;
            requestFullRender();
            break;

        case 1:
            if (ir->hasLastCapture()) {
                ir->replayLast();
                _mode = Mode::Captured;
                requestPartialRender();
            }
            break;

        case 2:
            if (_returnApp) {
                _appManager->switchTo(_returnApp);
            }
            break;
    }
}

void IrToolsApp::goBack() {
    if (!_appManager || !_returnApp) {
        return;
    }

    if (_mode == Mode::Recording || _mode == Mode::Captured) {
        _mode = Mode::Menu;
        requestFullRender();
        return;
    }

    _appManager->switchTo(_returnApp);
}

void IrToolsApp::drawMenu(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    for (int i = 0; i < ITEM_COUNT; i++) {
        const int y = rowBaselineY(i);

        display.drawText(CARET_X, y, (i == _selectedIndex) ? ">" : " ");
        display.drawText(LABEL_X, y, _items[i]);
    }
}

void IrToolsApp::drawCapturedInfo(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    if (!_appManager || !_appManager->context() || !_appManager->context()->ir) {
        display.drawText(INFO_X, INFO_Y1, "IR unavailable");
        return;
    }

    IrManager* ir = _appManager->context()->ir;

    if (_mode == Mode::Recording) {
        display.drawText(INFO_X, INFO_Y1, "Recording...");
        display.drawText(INFO_X, INFO_Y2, "Press remote button");
        display.drawText(INFO_X, INFO_Y4, "Back = cancel");
        return;
    }

    if (!ir->hasLastCapture()) {
        display.drawText(INFO_X, INFO_Y1, "No capture yet");
        return;
    }

    const IrCapture& cap = ir->lastCapture();

    String proto = "Proto: " + IrDriver::protocolToString(cap.protocol);
    String bits = "Bits : " + String(cap.bits);

    char valueBuf[32];
    snprintf(valueBuf, sizeof(valueBuf), "0x%llX",
             static_cast<unsigned long long>(cap.value));
    String value = "Value: " + String(valueBuf);

    display.drawText(INFO_X, INFO_Y1, "Captured");
    display.drawText(INFO_X, INFO_Y2, proto);
    display.drawText(INFO_X, INFO_Y3, bits);
    display.drawText(INFO_X, INFO_Y4, value);
}

void IrToolsApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(TITLE_X, TITLE_Y, "IR Tools");

        if (_mode == Mode::Menu) {
            drawMenu(display);
        } else {
            drawCapturedInfo(display);
        }
    } while (display.nextPage());

    Serial.println("[IrToolsApp] full render");
}

void IrToolsApp::renderPartial(DisplayManager& display) {
    int x = 0;
    int top = 50;
    int w = display.width();
    int h = display.height() - top;

    if (top % 2 != 0) top--;
    if (h % 2 != 0) h++;
    w = ((w + 7) / 8) * 8;

    display.startPartialWindowDraw(x, top, w, h);
    do {
        display.fillRectWhite(x, top, w, h);

        if (_mode == Mode::Menu) {
            drawMenu(display);
        } else {
            drawCapturedInfo(display);
        }
    } while (display.nextPage());

    Serial.println("[IrToolsApp] partial render");
}