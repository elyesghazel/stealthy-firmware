#include "MainMenuApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

namespace {
    constexpr int TITLE_X = 28;
    constexpr int TITLE_Y = 24;

    constexpr int MENU_TEXT_X = 100;
    constexpr int MENU_CARET_X = MENU_TEXT_X - 20;

    constexpr int FIRST_ROW_Y = 50;
    constexpr int ROW_SPACING = 22;
    constexpr int ROW_HEIGHT = 22;

}

MainMenuApp::MainMenuApp() {}

void MainMenuApp::setup(AppManager* appManager, IApp* badgeApp, IApp* aboutApp, IApp* settingsApp) {
    _appManager = appManager;
    _badgeApp = badgeApp;
    _aboutApp = aboutApp;
    _settingsApp = settingsApp;
}

void MainMenuApp::onEnter() {
    Serial.println("[MainMenuApp] enter");
    _selectedIndex = 0;
    _previousSelectedIndex = 0;
    _partialUpdateCount = 0;
    requestFullRender();
}

void MainMenuApp::onExit() {
    Serial.println("[MainMenuApp] exit");
}

void MainMenuApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) {
        return;
    }

    switch (event.id) {
        case ButtonId::Up:
            moveSelectionUp();
            break;

        case ButtonId::Down:
            moveSelectionDown();
            break;

        case ButtonId::Back:
                if (_appManager && _badgeApp) {
                    _appManager->switchTo(_badgeApp);
                }
            break;

        case ButtonId::Select:
            Serial.printf("[MainMenuApp] selected: %s\n", _items[_selectedIndex]);
            switch (_selectedIndex) {
                case 0:
                    if (_appManager && _badgeApp) {
                        _appManager->switchTo(_badgeApp);
                    }
                    break;

                case 1:
                case 2:
                    if (_appManager && _aboutApp) {
                        _appManager->switchTo(_aboutApp);
                    }
                    break;
                case 3:
                    if (_appManager && _settingsApp) {
                        _appManager->switchTo(_settingsApp);
                    }
                    break;
            }
        default:
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
    if (_selectedIndex <= 0) {
        return;
    }

    _selectedIndex--;

    _partialUpdateCount++;
    if (_partialUpdateCount >= 25) {
        _partialUpdateCount = 0;
        requestFullRender();
    } else {
        requestPartialRender();
    }
}

void MainMenuApp::moveSelectionDown() {
    if (_selectedIndex >= ITEM_COUNT - 1) {
        return;
    }

    _selectedIndex++;

    _partialUpdateCount++;
    if (_partialUpdateCount >= 25) {
        _partialUpdateCount = 0;
        requestFullRender();
    } else {
        requestPartialRender();
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

int MainMenuApp::rowBaselineY(int index) const {
    return FIRST_ROW_Y + index * ROW_SPACING;
}

int MainMenuApp::rowTopY(int index) const {
    return rowBaselineY(index) - 15;
}

void MainMenuApp::drawRow(DisplayManager& display, int index, bool selected) {
    const int y = rowBaselineY(index);

    display.setTextBlack();

    if (selected) {
        display.drawText(MENU_CARET_X, y, ">");
    } else {
        display.drawText(MENU_CARET_X, y, " ");
    }

    display.drawText(MENU_TEXT_X, y, _items[index]);
}

void MainMenuApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();

        display.setTitleFont();
        display.setTextBlack();
        display.drawText(TITLE_X, TITLE_Y, "Main Menu");

        display.setDefaultFont();
        for (int i = 0; i < ITEM_COUNT; i++) {
            drawRow(display, i, i == _selectedIndex);
        }
    } while (display.nextPage());

    Serial.println("[MainMenuApp] full render");
}

void MainMenuApp::renderPartial(DisplayManager& display) {
    const int menuTop = rowTopY(0);
    const int menuBottom = rowTopY(ITEM_COUNT - 1) + ROW_HEIGHT;
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

        display.setDefaultFont();
        display.setTextBlack();

        for (int i = 0; i < ITEM_COUNT; i++) {
            drawRow(display, i, i == _selectedIndex);
        }
    } while (display.nextPage());

    Serial.println("[MainMenuApp] partial render");
}