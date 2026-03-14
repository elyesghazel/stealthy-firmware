#pragma once

class DisplayManager;
class ButtonManager;
class PowerManager;
class StorageManager;
class IrManager;

struct AppContext {
    DisplayManager* display = nullptr;
    ButtonManager* buttons = nullptr;
    PowerManager* power = nullptr;
    StorageManager* storage = nullptr;
    IrManager* ir = nullptr;
};