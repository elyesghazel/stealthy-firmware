#include "IrToolsApp.h"
#include "../core/AppManager.h"
#include "../core/AppContext.h"
#include "../core/IrManager.h"
#include "../core/StorageManager.h"
#include "../drivers/DisplayManager.h"
#include "../drivers/ButtonManager.h"
#include "../core/LedManager.h"
#include "../drivers/IrDriver.h"
#include <cstdio>

namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;

    constexpr int CARET_X = 10;
    constexpr int LABEL_X = 24;

    constexpr int FIRST_ROW_Y = 56;
    constexpr int ROW_SPACING = 14;

    constexpr int INFO_X = 10;
    constexpr int INFO_Y1 = 64;
    constexpr int INFO_Y2 = 80;
    constexpr int INFO_Y3 = 96;
    constexpr int INFO_Y4 = 112;
    constexpr int INFO_Y5 = 126;

    constexpr int FOOTER_Y = 112;
    constexpr int SCROLL_TOP_Y = 54;
    constexpr int SCROLL_BOTTOM_Y = 104;
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
    _savedSelectedIndex = 0;
    _savedScrollOffset = 0;
    _partialUpdateCount = 0;
    refreshSavedItems();
    requestFullRender();
}

void IrToolsApp::onExit() {
    Serial.println("[IrToolsApp] exit");
}

void IrToolsApp::handleButton(const ButtonEvent& event) {
    if (event.id == ButtonId::Select && event.action == ButtonAction::LongPress) {
        if (_mode == Mode::Captured) {
            saveLastCapture();
        }
        return;
    }

    const bool moveEvent =
        (event.action == ButtonAction::Press ||
         event.action == ButtonAction::Repeat);

    switch (event.id) {
        case ButtonId::Up:
            if (moveEvent) {
                moveUp();
            }
            break;

        case ButtonId::Down:
            if (moveEvent) {
                moveDown();
            }
            break;

        case ButtonId::Select:
            if (event.action == ButtonAction::Press) {
                selectItem();
            }
            break;

        case ButtonId::Back:
            if (event.action == ButtonAction::Press) {
                goBack();
            }
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
        _appManager->context()->leds->showIrTransmit();
        _mode = Mode::Captured;
        requestFullRender();
    }

    if (_showSavedMessage && millis() > _saveFeedbackUntilMs) {
        _showSavedMessage = false;
        requestPartialRender();
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

void IrToolsApp::refreshSavedItems() {
    _savedItems.clear();

    if (!_appManager || !_appManager->context() || !_appManager->context()->storage) {
        return;
    }

    _savedItems = _appManager->context()->storage->listIrCaptures();

    if (_savedSelectedIndex >= static_cast<int>(_savedItems.size())) {
        _savedSelectedIndex = _savedItems.empty() ? 0 : static_cast<int>(_savedItems.size()) - 1;
    }

    clampSavedScroll();
}

void IrToolsApp::clampSavedScroll() {
    if (_savedSelectedIndex < _savedScrollOffset) {
        _savedScrollOffset = _savedSelectedIndex;
    }

    if (_savedSelectedIndex >= _savedScrollOffset + VISIBLE_ITEMS) {
        _savedScrollOffset = _savedSelectedIndex - VISIBLE_ITEMS + 1;
    }

    if (_savedScrollOffset < 0) {
        _savedScrollOffset = 0;
    }
}

void IrToolsApp::moveUp() {
    if (_mode == Mode::Menu) {
        if (_selectedIndex > 0) {
            _selectedIndex--;
        } else {
            _selectedIndex = MENU_ITEM_COUNT - 1;
        }

        _partialUpdateCount++;
        if (_partialUpdateCount >= 20) {
            _partialUpdateCount = 0;
            requestFullRender();
        } else {
            requestPartialRender();
        }
        return;
    }

    if (_mode == Mode::SavedList) {
        if (_savedItems.empty()) {
            return;
        }

        if (_savedSelectedIndex > 0) {
            _savedSelectedIndex--;
        } else {
            _savedSelectedIndex = static_cast<int>(_savedItems.size()) - 1;
        }

        clampSavedScroll();

        _partialUpdateCount++;
        if (_partialUpdateCount >= 20) {
            _partialUpdateCount = 0;
            requestFullRender();
        } else {
            requestPartialRender();
        }
    }
}

void IrToolsApp::moveDown() {
    if (_mode == Mode::Menu) {
        if (_selectedIndex < MENU_ITEM_COUNT - 1) {
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
        return;
    }

    if (_mode == Mode::SavedList) {
        if (_savedItems.empty()) {
            return;
        }

        if (_savedSelectedIndex < static_cast<int>(_savedItems.size()) - 1) {
            _savedSelectedIndex++;
        } else {
            _savedSelectedIndex = 0;
        }

        clampSavedScroll();

        _partialUpdateCount++;
        if (_partialUpdateCount >= 20) {
            _partialUpdateCount = 0;
            requestFullRender();
        } else {
            requestPartialRender();
        }
    }
}

void IrToolsApp::selectItem() {
    if (!_appManager || !_appManager->context()) {
        return;
    }

    IrManager* ir = _appManager->context()->ir;
    StorageManager* storage = _appManager->context()->storage;
    LedManager* leds = _appManager->context()->leds;

    if (!ir) {
        return;
    }

    if (!leds) {
        return;
    }


    if (_mode == Mode::Recording) {
        return;
    }

    if (_mode == Mode::Captured) {
        ir->replayLast();

        leds->showIrTransmit();
        requestPartialRender();
        return;
    }

    if (_mode == Mode::SavedList) {
        if (_savedItems.empty() || !storage) {
            return;
        }

        IrCapture loaded;
        int id = _savedItems[_savedSelectedIndex].id;

        if (storage->loadIrCaptureById(id, loaded)) {
            ir->setLastCapture(loaded);
            ir->replayLast();

            // stay in saved list so user can quickly send the next one
            requestPartialRender();
        }

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
            if (storage && ir->hasLastCapture()) {
                bool ok = storage->saveIrCapture(ir->lastCapture());
                Serial.println(ok ? "[IR] save ok" : "[IR] save failed");
                refreshSavedItems();
                _mode = Mode::Captured;
                requestFullRender();
            }
            break;

        case 3:
            refreshSavedItems();
            _mode = Mode::SavedList;
            requestFullRender();
            break;

        case 4:
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

    if (_mode == Mode::Recording || _mode == Mode::Captured || _mode == Mode::SavedList) {
        _mode = Mode::Menu;
        requestFullRender();
        return;
    }

    _appManager->switchTo(_returnApp);
}

void IrToolsApp::drawMenu(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        const int y = rowBaselineY(i);

        display.drawText(CARET_X, y, (i == _selectedIndex) ? ">" : " ");
        display.drawText(LABEL_X, y, _menuItems[i]);
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
    StorageManager* storage = _appManager->context()->storage;

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

    if (storage) {
        display.drawText(INFO_X, INFO_Y5, "Sel=replay/save menu");
    }
}

void IrToolsApp::drawSavedList(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    if (_savedItems.empty()) {
        display.drawText(INFO_X, INFO_Y1, "No saved IR codes");
        display.drawText(INFO_X, INFO_Y3, "Back = menu");
        return;
    }

    for (int visibleRow = 0; visibleRow < VISIBLE_ITEMS; visibleRow++) {
        int itemIndex = _savedScrollOffset + visibleRow;
        if (itemIndex >= static_cast<int>(_savedItems.size())) {
            break;
        }

        const int y = rowBaselineY(visibleRow);

        display.drawText(CARET_X, y, (itemIndex == _savedSelectedIndex) ? ">" : " ");
        display.drawText(LABEL_X, y, _savedItems[itemIndex].name);
    }

    if (_savedScrollOffset > 0) {
        display.drawText(6, SCROLL_TOP_Y, "^");
    }

    if (_savedScrollOffset + VISIBLE_ITEMS < static_cast<int>(_savedItems.size())) {
        display.drawText(6, SCROLL_BOTTOM_Y, "v");
    }

    display.drawText(10, FOOTER_Y, "Sel=replay  Back=menu");
}

void IrToolsApp::saveLastCapture() {
    if (!_appManager || !_appManager->context()) {
        return;
    }

    IrManager* ir = _appManager->context()->ir;
    StorageManager* storage = _appManager->context()->storage;
    LedManager* leds = _appManager->context()->leds;

    if (!ir || !storage || !ir->hasLastCapture()) {
        return;
    }

    bool ok = storage->saveIrCapture(ir->lastCapture());
    Serial.println(ok ? "[IR] save ok" : "[IR] save failed");

    if (ok) {
        refreshSavedItems();

        if (leds) {
            leds->showSuccess();
        }

        _showSavedMessage = true;
        _saveFeedbackUntilMs = millis() + 800;

        requestFullRender();
    }
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
        } else if (_mode == Mode::SavedList) {
            drawSavedList(display);
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
        } else if (_mode == Mode::SavedList) {
            drawSavedList(display);
        } else {
            drawCapturedInfo(display);
        }
    } while (display.nextPage());

    Serial.println("[IrToolsApp] partial render");
}