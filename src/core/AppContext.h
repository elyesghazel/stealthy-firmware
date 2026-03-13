#pragma once

class DisplayManager;
class ButtonManager;
class PowerManager;
class StorageManager;

struct AppContext {
    DisplayManager* display = nullptr;
    ButtonManager* buttons = nullptr;
    PowerManager* power = nullptr;
    StorageManager* storage = nullptr;
};