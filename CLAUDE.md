# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a PlatformIO project targeting the ESP32-S3 (Arduino framework).

```bash
# Build
platformio run

# Build and upload to device (/dev/ttyACM0 at 921600 baud)
platformio run --target upload

# Upload filesystem image (LittleFS)
platformio run --target uploadfs

# Open serial monitor (115200 baud)
platformio device monitor

# Build + upload + monitor in one step
platformio run --target upload && platformio device monitor
```

No tests are currently implemented. The `/test/` directory exists but is empty.

## Architecture

The firmware uses a **hierarchical app state machine** with a **service locator** (`AppContext`) injected into every app.

### Source layout

Code is organized by **domain** — each subsystem owns both its hardware driver and high-level manager in one folder:

```
src/
├── main.cpp
├── framework/   IApp interface, AppContext service locator, AppManager state machine
├── display/     DisplayManager (GxEPD2 e-ink wrapper)
├── input/       ButtonManager (debounce, long-press, event queue)
├── power/       PowerManager (battery ADC, deep sleep)
├── storage/     FileSystemDriver (LittleFS) + StorageManager (settings, IR files)
├── ir/          IrDriver (hardware) + IrManager (capture/replay orchestration)
├── led/         LedDriver (GPIO) + LedManager (animations state machine)
├── portal/      PortalManager (captive portal AP, REST API, web UI)
└── apps/        StartScreenApp, BadgeApp, MainMenuApp, SettingsApp,
                 AboutApp, IrToolsApp, PortalApp
```

`include/` holds compile-time constants: `AppInfo.h` (name/version) and `DeviceSettings.h` (persisted user prefs).

All `#include` paths are relative to `src/` (e.g. `"ir/IrManager.h"`, `"framework/IApp.h"`).

### App Lifecycle (`IApp` interface)

Every app implements:
- `onEnter()` / `onExit()` — called by `AppManager` on transitions
- `handleButton(ButtonEvent)` — process input events
- `update()` — logic tick (called each loop iteration)
- `render(DisplayManager*)` — draw to display

Apps track `_needsRender` and a render mode enum (Full vs. Partial) to minimize costly e-ink refreshes.

### Key Patterns

**AppContext (service locator):** All managers (`DisplayManager`, `ButtonManager`, `PowerManager`, `StorageManager`, `IrManager`, `LedManager`, `PortalManager`) are bundled in `AppContext` and passed to every app — never use globals.

**ButtonManager event queue:** Circular buffer of `ButtonEvent`s with debouncing (25ms), long-press detection (700ms), and auto-repeat for Up/Down (300ms start, 110ms interval).

**E-ink rendering:** The display requires a page-loop pattern. Use `startFullWindowDraw()` or `startPartialWindowDraw(x, y, w, h)` then loop calling `nextPage()` until it returns false. Prefer partial updates to avoid full-screen flicker.

**Power management:** Main loop runs every 10ms. `PowerManager` tracks inactivity; default sleep timeout is 5 minutes. Deep sleep is entered via `enterDeepSleep()` with wakeup on EXT1 GPIO interrupt. Call `context.power->notifyActivity()` on any user interaction.

**Storage layout (LittleFS):**
- `/config/` — badge name and status
- `/ir/` — IR captures (text/JSON-like format)
- `/payloads/` — arbitrary payload files

**PortalManager:** Creates a captive portal AP (`STEALTHY-SETUP`, IP `192.168.4.1`) with a web UI and REST API for configuration, IR management, and file download.

### Main Loop Order (main.cpp)

1. `ButtonManager.update()` + dispatch events to current app
2. `PowerManager.update()` + check sleep timeout
3. `LedManager.update()` (LED state machine)
4. `PortalManager.update()` (DNS + web server)
5. `AppManager.update()` + `AppManager.render()`
6. Sleep if triggered

## Hardware

- **MCU:** ESP32-S3 (4D Systems Gen4 R8N16)
- **Display:** 2.13" e-ink (GxEPD2, partial-update capable)
- **IR:** TX pin 14, RX pin 21 (IRremoteESP8266 library)
- **LEDs:** Blue + green (LedDriver)
- **Buttons:** 4 buttons — Up, Down, Select, Back
- **Filesystem:** LittleFS
- **Upload port:** `/dev/ttyACM0`
