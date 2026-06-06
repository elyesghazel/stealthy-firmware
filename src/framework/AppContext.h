#pragma once

class DisplayManager;
class ButtonManager;
class PowerManager;
class StorageManager;
class IrManager;
class LedManager;
class PortalManager;
class WifiSpammer;
class WifiDeauth;

struct AppContext {
    DisplayManager* display  = nullptr;
    ButtonManager*  buttons  = nullptr;
    PowerManager*   power    = nullptr;
    StorageManager* storage  = nullptr;
    IrManager*      ir       = nullptr;
    LedManager*     leds     = nullptr;
    PortalManager*  portal   = nullptr;
    WifiSpammer*    spammer  = nullptr;
    WifiDeauth*     deauth   = nullptr;
};
