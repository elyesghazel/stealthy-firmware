#include "MainMenuApp.h"
#include "core/AppManager.h"
#include "drivers/DisplayManager.h"

MainMenuApp::MainMenuApp() {}

void MainMenuApp::setup(AppManager* appManager, IApp* badgeApp) {
    _appManager = appManager;
    _badgeApp = badgeApp;
}


void MainMenuApp::onEnter() {
    Serial.println("[MainMenuApp] enter");
    _selectedIndex = 0;
    _previousSelectedIndex = 0;
    _needsRender = true;
    _partialUpdateCount = 0;
    _renderMode = MenuRenderMode::Full;
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
            if (_selectedIndex > 0) {
                _previousSelectedIndex = _selectedIndex;
                _selectedIndex--;
                _needsRender = true;
                _renderMode = MenuRenderMode::Partial;
            }
            break;

        case ButtonId::Down:
            if (_selectedIndex < ITEM_COUNT - 1) {
                _previousSelectedIndex = _selectedIndex;
                _selectedIndex++;
                _needsRender = true;
                _renderMode = MenuRenderMode::Partial;
            }
            break;

        case ButtonId::Back:
            if (_appManager && _badgeApp) {
                _appManager->switchTo(_badgeApp);
            }
            break;

        case ButtonId::Select:
            Serial.printf("[MainMenuApp] selected: %s\n", _items[_selectedIndex]);
            break;
    }
}

void MainMenuApp::update() {
}

void MainMenuApp::render(DisplayManager& display) {
    if (!_needsRender) {
        return;
    }

    _partialUpdateCount++;
    if (_partialUpdateCount >= 10) {
        _renderMode = MenuRenderMode::Full;
        _needsRender = true;
        _partialUpdateCount = 0;
    }

    if (_renderMode == MenuRenderMode::Full) {
        display.startFullWindowDraw();
        do {
            display.fillWhite();

            display.setTitleFont();
            display.setTextBlack();
            display.drawText(10, 20, "Main Menu");

            display.setDefaultFont();

            for (int i = 0; i < ITEM_COUNT; i++) {
                int y = 45 + i * 18;

                int16_t x1, y1;
                uint16_t w, h;
                display.getTextBounds(_items[i], 10, y, &x1, &y1, &w, &h);

                if (i == _selectedIndex) {
                    display.fillRect(x1 - 4, y1 - 2, w + 8, h + 4);
                    display.setTextWhite();
                    display.drawText(10, y, _items[i]);
                    display.setTextBlack();
                } else {
                    display.drawText(10, y, _items[i]);
                }
            }
        } while (display.nextPage());

        Serial.println("[MainMenuApp] full render");
    } else {
        int oldY = 45 + _previousSelectedIndex * 18;
        int newY = 45 + _selectedIndex * 18;

        int16_t oldX1, oldY1, newX1, newY1;
        uint16_t oldW, oldH, newW, newH;

        display.setDefaultFont();
        display.getTextBounds(_items[_previousSelectedIndex], 10, oldY, &oldX1, &oldY1, &oldW, &oldH);
        display.getTextBounds(_items[_selectedIndex], 10, newY, &newX1, &newY1, &newW, &newH);

        int left = min(oldX1, newX1) - 4;
        int top = min(oldY1, newY1) - 2;
        int right = max(oldX1 + (int)oldW, newX1 + (int)newW) + 4;
        int bottom = max(oldY1 + (int)oldH, newY1 + (int)newH) + 2;

        int width = right - left;
        int height = bottom - top;

        display.startPartialWindowDraw(left, top, width, height);
        do {
            display.fillRectWhite(left, top, width, height);

            display.setDefaultFont();

            display.setTextBlack();
            display.drawText(10, oldY, _items[_previousSelectedIndex]);

            display.getTextBounds(_items[_selectedIndex], 10, newY, &newX1, &newY1, &newW, &newH);
            display.fillRect(newX1 - 4, newY1 - 2, newW + 8, newH + 4);
            display.setTextWhite();
            display.drawText(10, newY, _items[_selectedIndex]);
            display.setTextBlack();
        } while (display.nextPage());

        Serial.println("[MainMenuApp] partial render");
    }

    _needsRender = false;
}