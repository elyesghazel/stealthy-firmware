# Stealthy Firmware

ESP32-S3 hacker badge. E-ink display, IR transceiver, WiFi attack tools, captive portal web UI.

**Hardware:** 4D Systems Gen4 R8N16 (ESP32-S3) · **FW:** v0.1.0

---

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
platformio run --target upload          # flash firmware
platformio run --target uploadfs        # flash web UI + LittleFS (do this on first flash)
platformio device monitor               # serial monitor (115200)
```

---

## Hardware Pins

| Function | GPIO |
|----------|------|
| Button UP / DOWN / SELECT / BACK | 10 / 11 / 12 / 13 |
| LED Blue / Green | 2 / 1 |
| IR TX / RX | 14 / 21 |
| Battery ADC | 9 |
| Wake from sleep | GPIO 12 (RTC EXT1) |

Display: 2.13" GxEPD2 e-ink, 250×122 px. Filesystem: LittleFS.

---

## Navigation

Four buttons: **Up**, **Down**, **Select**, **Back**.

| Shortcut | Action |
|----------|--------|
| Long press **Back** | Go to Main Menu (from anywhere) |
| Long press **Back** on Main Menu | Go to Badge display |
| Long press **Select** on Main Menu | Toggle portal on/off |
| Long press **Select** in IR Tools (Captured mode) | Save capture |

Auto-repeat on Up/Down kicks in after 100 ms, fires every 40 ms.

---

## Apps

**Badge** — Main display face. Shows name, tagline, battery, IR count, uptime, QR code.

**Main Menu** — Hub. Items: Badge · Settings · About · Captive Portal · IR Tools · WiFi Tools.

**Settings** — Sleep timeout, refresh interval, badge status, start screen toggle. Changes apply and persist immediately.

**IR Tools** — Record IR signals, replay, save, browse saved captures. Supports Flipper Zero `.ir` file format (upload via portal, browse/send on-device).

**Portal** — Starts the captive WiFi AP (`STEALTHY-SETUP` · `192.168.4.1`). Full web UI for remote control. Prevents deep sleep while a client is connected.

**WiFi Tools → Beacon Flood** — Broadcasts fake SSIDs. Configure via portal Flood tab.

**WiFi Tools → Deauth** — Scans for nearby APs, sends 802.11 deauth frames at a selected target.

**Portal → Recon tab** — Passive probe sniffer (what SSIDs devices are looking for) + Karma AP (clone a probed SSID as a rogue AP, redirect victims to captive portal).

---

## Captive Portal Web UI

Connect to **STEALTHY-SETUP** → open any URL → redirected to the dashboard.

| Tab | What you can do |
|-----|----------------|
| **System** | Battery status · Portal autostart toggle · Sleep timeout · Start screen toggle |
| **Badge** | Set name (max 12 chars), tagline, status, QR data |
| **IR Library** | Send / rename / export / delete saved captures |
| **IR Upload** | Upload Flipper `.ir` files · browse signals · send remotely · import to library |
| **Flood** | Edit beacon flood SSID list (max 20) |
| **Recon** | Probe sniffer · Karma AP |

---

## Storage (LittleFS)

```
/config/          — name, status, tagline, qr_data, badge_mode,
                    spam_ssids, portal_autostart, device_settings
/ir/saved/        — captured IR signals (0001.ir + 0001.txt pairs)
/ir/uploads/      — Flipper Zero .ir files
/payloads/        — arbitrary files
```

---

## Architecture

```
src/framework/    IApp interface + AppManager state machine + AppContext service locator
src/apps/         One file per screen app
src/display/      GxEPD2 wrapper (full + partial page-loop rendering)
src/input/        Button debounce, long-press, auto-repeat, event queue
src/power/        Battery ADC, deep sleep, sleep timeout
src/storage/      LittleFS driver + settings/IR file management
src/ir/           IRremoteESP8266 capture/replay + Flipper format parser/serializer
src/led/          LED animation state machine
src/portal/       WebServer + DNSServer + REST API
src/wifi/         WifiSpammer, WifiDeauth, WifiKarma
```

All managers are passed to every app via `AppContext` (service locator). Apps are singletons instantiated once at boot; `AppManager::switchTo()` calls `onExit()` / `onEnter()` on transitions.

E-ink rendering uses a **page-loop** pattern — always wrap draw calls between `startFullWindowDraw()` / `startPartialWindowDraw()` and `while (display.nextPage())`.

---

## Adding an App

1. Subclass `IApp`, implement `onEnter`, `onExit`, `handleButton`, `update`, `render`.
2. Access any manager via `_appManager->context()->storage` / `->ir` / `->power` / etc.
3. Instantiate at file scope in `main.cpp`, call `setup()` after other apps, wire into a parent menu.

See any existing app in `src/apps/` for a working example.
