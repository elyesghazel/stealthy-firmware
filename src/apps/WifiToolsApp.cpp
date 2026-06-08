#include "WifiToolsApp.h"
#include "framework/AppManager.h"
#include "display/DisplayManager.h"

namespace {
    constexpr int TITLE_X        = 5;
    constexpr int TITLE_Y        = 38;
    constexpr int CARET_X        = 72;
    constexpr int LABEL_X        = 90;
    constexpr int FIRST_ROW_Y    = 68;
    constexpr int ROW_SPACING    = 18;
    constexpr int ROW_HEIGHT     = 18;
}

void WifiToolsApp::setup(AppManager* appManager, IApp* returnApp,
                          IApp* spammerApp, IApp* deauthApp, IApp* appleJuiceApp) {
    _appManager    = appManager;
    _returnApp     = returnApp;
    _spammerApp    = spammerApp;
    _deauthApp     = deauthApp;
    _appleJuiceApp = appleJuiceApp;
}

void WifiToolsApp::onEnter() {
    _selectedIndex  = 0;
    _partialUpdates = 0;
    requestFullRender();
    Serial.println("[WifiToolsApp] enter");
}

void WifiToolsApp::onExit() {
    Serial.println("[WifiToolsApp] exit");
}

void WifiToolsApp::handleButton(const ButtonEvent& event) {
    const bool moveEvent =
        (event.action == ButtonAction::Press || event.action == ButtonAction::Repeat);

    switch (event.id) {
        case ButtonId::Up:
            if (moveEvent) {
                _selectedIndex = (_selectedIndex == 0) ? ITEM_COUNT - 1 : _selectedIndex - 1;
                _partialUpdates++;
                if (_partialUpdates >= 20) { _partialUpdates = 0; requestFullRender(); }
                else requestPartialRender();
            }
            break;

        case ButtonId::Down:
            if (moveEvent) {
                _selectedIndex = (_selectedIndex == ITEM_COUNT - 1) ? 0 : _selectedIndex + 1;
                _partialUpdates++;
                if (_partialUpdates >= 20) { _partialUpdates = 0; requestFullRender(); }
                else requestPartialRender();
            }
            break;

        case ButtonId::Back:
            if (event.action == ButtonAction::Press && _appManager && _returnApp) {
                _appManager->switchTo(_returnApp);
            }
            break;

        case ButtonId::Select:
            if (event.action != ButtonAction::Press || !_appManager) break;
            if (_selectedIndex == 0 && _spammerApp)    _appManager->switchTo(_spammerApp);
            if (_selectedIndex == 1 && _deauthApp)     _appManager->switchTo(_deauthApp);
            if (_selectedIndex == 2 && _appleJuiceApp) _appManager->switchTo(_appleJuiceApp);
            break;
    }
}

void WifiToolsApp::update() {}

void WifiToolsApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    if (_fullRender) renderFull(display);
    else             renderPartial(display);
    _needsRender = false;
}

void WifiToolsApp::requestFullRender() {
    _fullRender = true; _needsRender = true;
}

void WifiToolsApp::requestPartialRender() {
    _fullRender = false; _needsRender = true;
}

int WifiToolsApp::rowBaselineY(int row) const {
    return FIRST_ROW_Y + row * ROW_SPACING;
}

int WifiToolsApp::rowTopY(int row) const {
    return rowBaselineY(row) - 13;
}

void WifiToolsApp::drawMenu(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    for (int r = 0; r < ITEM_COUNT; r++) {
        int y = rowBaselineY(r);
        if (r == _selectedIndex) {
            display.fillRect(CARET_X - 2, y - 2, display.width() - (CARET_X - 2), 12);
            display.setTextWhite();
            display.drawText(CARET_X, y, ">");
            display.drawText(LABEL_X, y, _items[r]);
            display.setTextBlack();
        } else {
            display.drawText(LABEL_X, y, _items[r]);
        }
    }
}

void WifiToolsApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        display.setTitleFont();
        display.setTextBlack();
        display.drawText(TITLE_X, TITLE_Y, "WiFi Tools");
        drawMenu(display);
    } while (display.nextPage());
}

void WifiToolsApp::renderPartial(DisplayManager& display) {
    int top = rowTopY(0) - 6;
    int bot = rowTopY(ITEM_COUNT - 1) + ROW_HEIGHT + 6;
    int h   = bot - top;
    int x   = 0;
    int w   = display.width();

    if (top < 0) top = 0;
    if (top % 2 != 0) top--;
    if (h % 2 != 0) h++;
    w = ((w + 7) / 8) * 8;

    display.startPartialWindowDraw(x, top, w, h);
    do {
        display.fillRectWhite(x, top, w, h);
        drawMenu(display);
    } while (display.nextPage());
}
