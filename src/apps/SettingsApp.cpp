#include "SettingsApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;

    constexpr int CARET_X = 6;
    constexpr int LABEL_X = 18;
    constexpr int VALUE_X = 150;

    constexpr int FIRST_ROW_Y = 64;
    constexpr int ROW_SPACING = 16;
    constexpr int ROW_HEIGHT = 16;

    constexpr int VISIBLE_ITEMS = 4;

    constexpr int FOOTER_Y = 116;
    constexpr int SCROLL_TOP_Y = 56;
    constexpr int SCROLL_BOTTOM_Y = 112;
}

SettingsApp::SettingsApp() {}

void SettingsApp::setup(AppManager* appManager, IApp* returnApp, DeviceSettings* settings) {
    _appManager = appManager;
    _returnApp = returnApp;
    _settings = settings;
}

void SettingsApp::requestFullRender() {
    _renderMode = SettingsRenderMode::Full;
    _needsRender = true;
}

void SettingsApp::requestPartialRender() {
    _renderMode = SettingsRenderMode::Partial;
    _needsRender = true;
}

int SettingsApp::rowBaselineY(int index) const {
    return FIRST_ROW_Y + index * ROW_SPACING;
}

int SettingsApp::rowTopY(int index) const {
    return rowBaselineY(index) - 14;
}

void SettingsApp::onEnter() {
    Serial.println("[SettingsApp] enter");
    _selectedIndex = 0;
    _previousSelectedIndex = 0;
    _scrollOffset = 0;
    _partialUpdateCount = 0;
    _mode = SettingsMode::Browse;
    requestFullRender();
}

void SettingsApp::onExit() {
    Serial.println("[SettingsApp] exit");
}

void SettingsApp::handleButton(const ButtonEvent& event) {
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

void SettingsApp::update() {
}

void SettingsApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    if (_renderMode == SettingsRenderMode::Full) {
        renderFull(display);
    } else {
        renderPartial(display);
    }

    _needsRender = false;
}
void SettingsApp::clampScrollToSelection() {
    if (_selectedIndex < _scrollOffset) {
        _scrollOffset = _selectedIndex;
    }

    if (_selectedIndex >= _scrollOffset + VISIBLE_ITEMS) {
        _scrollOffset = _selectedIndex - VISIBLE_ITEMS + 1;
    }
}

void SettingsApp::drawScrollIndicators(DisplayManager& display) {
    display.setTextBlack();

    if (_scrollOffset > 0) {
        display.drawText(6, SCROLL_TOP_Y, "^");
    }
    
    if (_scrollOffset + VISIBLE_ITEMS < ITEM_COUNT) {
        display.drawText(6, SCROLL_BOTTOM_Y, "v");
    }
}

void SettingsApp::drawValue(DisplayManager& display, int index, int y) {
    const char* value = currentValueText(index);

    if (_mode == SettingsMode::Edit && index == _selectedIndex) {
        String wrapped = "[" + String(value) + "]";
        display.drawText(VALUE_X, y, wrapped);
    } else {
        display.drawText(VALUE_X, y, value);
    }
}

void SettingsApp::drawSettingsList(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    for (int visibleRow = 0; visibleRow < VISIBLE_ITEMS; visibleRow++) {
        int itemIndex = _scrollOffset + visibleRow;
        if (itemIndex >= ITEM_COUNT) {
            break;
        }

        const int y = rowBaselineY(visibleRow);

        if (itemIndex == _selectedIndex) {
            display.drawText(CARET_X, y, ">");
        } else {
            display.drawText(CARET_X, y, " ");
        }

        display.drawText(LABEL_X, y, _items[itemIndex]);

        if (itemIndex < ITEM_COUNT - 1) {
            drawValue(display, itemIndex, y);
        }
    }

    drawScrollIndicators(display);
}

void SettingsApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(TITLE_X, TITLE_Y, "Settings");

        drawSettingsList(display);
    } while (display.nextPage());

    Serial.println("[SettingsApp] full render");
}

void SettingsApp::renderPartial(DisplayManager& display) {
    int top = rowTopY(0);
    int bottom = FOOTER_Y + 8;
    int height = bottom - top;

    int x = 0;
    int w = display.width();

    if (top < 0) {
        top = 0;
    }
    if (top % 2 != 0) {
        top--;
    }
    if (height % 2 != 0) {
        height++;
    }

    x = (x / 8) * 8;
    w = ((w + 7) / 8) * 8;

    display.startPartialWindowDraw(x, top, w, height);
    do {
        display.fillRectWhite(x, top, w, height);
        drawSettingsList(display);
    } while (display.nextPage());

    Serial.println("[SettingsApp] partial render");
}

void SettingsApp::moveUp() {
    if (!_settings) {
        return;
    }

    if (_mode == SettingsMode::Browse) {
        _previousSelectedIndex = _selectedIndex;

        if (_selectedIndex > 0) {
            _selectedIndex--;
        } else {
            _selectedIndex = ITEM_COUNT - 1;
        }

        clampScrollToSelection();

        _partialUpdateCount++;
        if (_partialUpdateCount >= 20) {
            _partialUpdateCount = 0;
            requestFullRender();
        } else {
            requestPartialRender();
        }

        return;
    }

    switch (_selectedIndex) {
        case 0:
            if (_settings->sleepTimeoutIndex > 0) {
                _settings->sleepTimeoutIndex--;
                requestPartialRender();
            }
            break;

        case 1:
            if (_settings->refreshIntervalIndex > 0) {
                _settings->refreshIntervalIndex--;
                requestPartialRender();
            }
            break;

        case 2:
            if (_settings->badgeStatusIndex > 0) {
                _settings->badgeStatusIndex--;
                requestPartialRender();
            }
            break;

        case 3:
            _settings->showStartScreen = !_settings->showStartScreen;
            requestPartialRender();
            break;

        default:
            break;
    }
}

void SettingsApp::moveDown() {
    if (!_settings) {
        return;
    }

    if (_mode == SettingsMode::Browse) {
        _previousSelectedIndex = _selectedIndex;

        if (_selectedIndex < ITEM_COUNT - 1) {
            _selectedIndex++;
        } else {
            _selectedIndex = 0;
        }

        clampScrollToSelection();

        _partialUpdateCount++;
        if (_partialUpdateCount >= 20) {
            _partialUpdateCount = 0;
            requestFullRender();
        } else {
            requestPartialRender();
        }

        return;
    }

    switch (_selectedIndex) {
        case 0:
            if (_settings->sleepTimeoutIndex < 3) {
                _settings->sleepTimeoutIndex++;
                requestPartialRender();
            }
            break;

        case 1:
            if (_settings->refreshIntervalIndex < 2) {
                _settings->refreshIntervalIndex++;
                requestPartialRender();
            }
            break;

        case 2:
            if (_settings->badgeStatusIndex < 3) {
                _settings->badgeStatusIndex++;
                requestPartialRender();
            }
            break;

        case 3:
            _settings->showStartScreen = !_settings->showStartScreen;
            requestPartialRender();
            break;

        default:
            break;
    }
}

void SettingsApp::selectItem() {
    if (!_appManager || !_returnApp) {
        return;
    }

    if (_selectedIndex == ITEM_COUNT - 1) {
        _appManager->switchTo(_returnApp);
        return;
    }

    if (_mode == SettingsMode::Browse) {
        _mode = SettingsMode::Edit;
    } else {
        _mode = SettingsMode::Browse;
    }

    requestPartialRender();
}

void SettingsApp::goBack() {
    if (!_appManager || !_returnApp) {
        return;
    }

    if (_mode == SettingsMode::Edit) {
        _mode = SettingsMode::Browse;
        requestPartialRender();
        return;
    }

    _appManager->switchTo(_returnApp);
}

const char* SettingsApp::currentValueText(int index) const {
    if (!_settings) {
        return "-";
    }

    switch (index) {
        case 0: {
            static const char* options[] = {"10s", "30s", "1m", "3m"};
            return options[_settings->sleepTimeoutIndex];
        }

        case 1: {
            static const char* options[] = {"10", "25", "50"};
            return options[_settings->refreshIntervalIndex];
        }

        case 2: {
            static const char* options[] = {"Online", "Busy", "Away", "Offline"};
            return options[_settings->badgeStatusIndex];
        }

        case 3:
            return _settings->showStartScreen ? "On" : "Off";

        default:
            return "";
    }
}