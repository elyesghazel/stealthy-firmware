#include "MainMenuApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

namespace {
    constexpr int TITLE_X = 5;
    constexpr int TITLE_Y = 38;

    constexpr int MENU_TEXT_X = 90;
    constexpr int MENU_CARET_X = MENU_TEXT_X - 18;

    constexpr int FIRST_ROW_Y = 64;
    constexpr int ROW_SPACING = 16;
    constexpr int ROW_HEIGHT = 16;

    constexpr int SCROLL_INDICATOR_X = 8;
    constexpr int SCROLL_TOP_Y = 56;
    constexpr int SCROLL_BOTTOM_Y = 132;
}

MainMenuApp::MainMenuApp() {}

void MainMenuApp::setup(
    AppManager* appManager,
    IApp* badgeApp,
    IApp* settingsApp,
    IApp* aboutApp,
    IApp* irToolsApp,
    IApp* portalApp
) {
    _appManager = appManager;
    _badgeApp = badgeApp;
    _settingsApp = settingsApp;
    _aboutApp = aboutApp;
    _irToolsApp = irToolsApp;
    _portalApp = portalApp;
}


void MainMenuApp::onEnter() {
    Serial.println("[MainMenuApp] enter");
    _selectedIndex = 0;
    _scrollOffset = 0;
    _partialUpdateCount = 0;
    requestFullRender();
}

void MainMenuApp::onExit() {
    Serial.println("[MainMenuApp] exit");
}

void MainMenuApp::handleButton(const ButtonEvent& event) {
    const bool moveEvent =
        (event.action == ButtonAction::Press ||
         event.action == ButtonAction::Repeat);

    switch (event.id) {
        case ButtonId::Up:
            if (moveEvent) {
                moveSelectionUp();
            }
            break;

        case ButtonId::Down:
            if (moveEvent) {
                moveSelectionDown();
            }
            break;

        case ButtonId::Back:
            if (event.action == ButtonAction::Press) {
                if (_appManager && _badgeApp) {
                    _appManager->switchTo(_badgeApp);
                }
            }
            break;

        case ButtonId::Select:
            if (event.action != ButtonAction::Press) {
                return;
            }

            if (!_appManager) {
                return;
            }

            switch (_selectedIndex) {
                case 0:
                    if (_badgeApp) _appManager->switchTo(_badgeApp);
                    break;

                case 1:
                    if (_settingsApp) _appManager->switchTo(_settingsApp);
                    break;

                case 2:
                    if (_aboutApp) _appManager->switchTo(_aboutApp);
                    break;

                case 3:
                    if (_portalApp) _appManager->switchTo(_portalApp);
                    break;

                case 4:
                    if (_irToolsApp) _appManager->switchTo(_irToolsApp);
                    break;
                
                case 5:
                    if (_badgeApp) _appManager->switchTo(_badgeApp);
                    break;
            }
            break;
    }
}

void MainMenuApp::update() {
}

void MainMenuApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    if (_renderMode == MenuRenderMode::Full) {
        renderFull(display);
    } else {
        renderPartial(display);
    }

    _needsRender = false;
}

void MainMenuApp::moveSelectionUp() {
    if (_selectedIndex > 0) {
        _selectedIndex--;
    } else {
        _selectedIndex = ITEM_COUNT - 1;
    }

    clampScrollToSelection();

    _partialUpdateCount++;
    if (_partialUpdateCount >= 25) {
        _partialUpdateCount = 0;
        requestFullRender();
    } else {
        requestPartialRender();
    }
}

void MainMenuApp::moveSelectionDown() {
    if (_selectedIndex < ITEM_COUNT - 1) {
        _selectedIndex++;
    } else {
        _selectedIndex = 0;
    }

    clampScrollToSelection();

    _partialUpdateCount++;
    if (_partialUpdateCount >= 25) {
        _partialUpdateCount = 0;
        requestFullRender();
    } else {
        requestPartialRender();
    }
}

void MainMenuApp::clampScrollToSelection() {
    if (_selectedIndex < _scrollOffset) {
        _scrollOffset = _selectedIndex;
    }

    if (_selectedIndex >= _scrollOffset + VISIBLE_ITEMS) {
        _scrollOffset = _selectedIndex - VISIBLE_ITEMS + 1;
    }
}

void MainMenuApp::requestFullRender() {
    _renderMode = MenuRenderMode::Full;
    _needsRender = true;
}

void MainMenuApp::requestPartialRender() {
    _renderMode = MenuRenderMode::Partial;
    _needsRender = true;
}

int MainMenuApp::rowBaselineY(int visibleRow) const {
    return FIRST_ROW_Y + visibleRow * ROW_SPACING;
}

int MainMenuApp::rowTopY(int visibleRow) const {
    return rowBaselineY(visibleRow) - 13;
}

void MainMenuApp::drawRow(DisplayManager& display, int visibleRow, int itemIndex, bool selected) {
    const int y = rowBaselineY(visibleRow);

    display.setTextBlack();

    if (selected) {
        display.drawText(MENU_CARET_X, y, ">");
    } else {
        display.drawText(MENU_CARET_X, y, " ");
    }

    display.drawText(MENU_TEXT_X, y, _items[itemIndex]);
}

void MainMenuApp::drawScrollIndicators(DisplayManager& display) {
    display.setTextBlack();

    if (_scrollOffset > 0) {
        display.drawText(SCROLL_INDICATOR_X, SCROLL_TOP_Y, "^");
    }

    if (_scrollOffset + VISIBLE_ITEMS < ITEM_COUNT) {
        display.drawText(SCROLL_INDICATOR_X, SCROLL_BOTTOM_Y, "v");
    }
}

void MainMenuApp::drawMenuItems(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    for (int visibleRow = 0; visibleRow < VISIBLE_ITEMS; visibleRow++) {
        int itemIndex = _scrollOffset + visibleRow;
        if (itemIndex >= ITEM_COUNT) {
            break;
        }

        drawRow(display, visibleRow, itemIndex, itemIndex == _selectedIndex);
    }

    drawScrollIndicators(display);
}

void MainMenuApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(TITLE_X, TITLE_Y, "Main Menu");

        drawMenuItems(display);
    } while (display.nextPage());

    Serial.println("[MainMenuApp] full render");
}

void MainMenuApp::renderPartial(DisplayManager& display) {
    const int menuTop = rowTopY(0) - 8;
    const int menuBottom = rowTopY(VISIBLE_ITEMS - 1) + ROW_HEIGHT + 18;
    int height = menuBottom - menuTop;

    int x = 0;
    int w = display.width();

    if (x % 8 != 0) {
        x = (x / 8) * 8;
    }
    w = ((w + 7) / 8) * 8;

    int top = menuTop;
    if (top < 0) top = 0;
    if (top % 2 != 0) top--;

    if (height % 2 != 0) height++;

    display.startPartialWindowDraw(x, top, w, height);
    do {
        display.fillRectWhite(x, top, w, height);
        drawMenuItems(display);
    } while (display.nextPage());

    Serial.println("[MainMenuApp] partial render");
}