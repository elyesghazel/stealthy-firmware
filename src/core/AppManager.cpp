#include "AppManager.h"
#include "drivers/DisplayManager.h"

void AppManager::begin(IApp* initialApp, AppContext* context) {
    _currentApp = initialApp;
    _context = context;
    if (_currentApp) {
        _currentApp->onEnter();
    }
}

AppContext* AppManager::context() const {
    return _context;
}

void AppManager::switchTo(IApp* nextApp) {
    if (_currentApp) {
        _currentApp->onExit();
    }

    _currentApp = nextApp;

    if (_currentApp) {
        _currentApp->onEnter();
    }
}

void AppManager::handleButton(const ButtonEvent& event) {
    if (_currentApp) {
        _currentApp->handleButton(event);
    }
}

void AppManager::update() {
    if (_currentApp) {
        _currentApp->update();
    }
}

void AppManager::render(DisplayManager& display) {
    if (_currentApp) {
        _currentApp->render(display);
    }
}

IApp* AppManager::currentApp() const {
    return _currentApp;
}