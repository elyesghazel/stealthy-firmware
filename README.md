# Stealthy Firmware

ESP32-S3 hacker badge. E-ink display, IR transceiver, WiFi attack tools, TOTP authenticator, captive portal web UI.

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

**WiFi Tools → Beacon Flood + Karma** — See below.

**WiFi Tools → Deauth** — Scans for nearby APs, sends 802.11 deauth frames at a selected target.

**Portal → Recon tab** — Passive probe sniffer (what SSIDs devices are looking for) + Karma AP (clone a probed SSID as a rogue AP, redirect victims to captive portal).

---

## Captive Portal Web UI

Connect to **STEALTHY-SETUP** → open any URL → redirected to the dashboard.

The portal is password-protected. Set a password in the System tab; leave it blank for open access.

| Tab | What you can do |
|-----|----------------|
| **System** | Battery · autostart · sleep timeout · start screen · portal password |
| **Profiles** | Multiple named profiles (name, tagline, QR data) · set active profile on device |
| **Badge** | Set badge name (max 12 chars), tagline, status, QR data |
| **IR Library** | Send / rename / export / delete saved captures |
| **IR Upload** | Upload Flipper `.ir` files · browse signals · send remotely · import to library |
| **Flood** | Beacon flood SSID list · Troll Captive Portal config |
| **Recon** | Probe sniffer · Karma AP |
| **TOTP** | Offline 2FA codes — add accounts by name + base32 secret, time synced from browser |

---

## Beacon Flood + Karma Troll Portal

These two features work together as a layered WiFi attack.

### Beacon Flood

Broadcasts fake 802.11 beacon frames for every SSID in your list (max 20). Nearby devices see all of them in their WiFi scanner — but they are radio-only broadcasts with no real AP behind them. Nobody can actually connect at this stage.

### Karma + Troll Portal

When you hit **Start** on the device, the beacon flood runs alongside a real AP running a karma attack:

1. The device listens in promiscuous mode for 802.11 probe requests on channel 6
2. When a nearby device probes for any SSID from your flood list, the AP instantly renames itself to match that SSID and sends a spoofed probe response back
3. The device sees a matching network and connects
4. DNS redirects all their traffic to the troll page (configurable HTML in the Flood tab)

The AP shape-shifts to match whoever is actively probing — one victim connects, gets the troll page, leaves, and the AP morphs to catch the next probe.

Configure in the **Flood** tab of the portal:
- **SSID list** — the networks that get beaconed and karma-matched
- **Troll AP SSID** — initial AP name before the first probe match (defaults to first SSID if left blank)
- **Troll Page HTML** — what victims see; a default "DEVICE INDEXED" terminal page is built in

---

## TOTP Authenticator

Offline 2FA codes stored on the device, displayed on the e-ink screen. Time is synced from your browser every time you open the portal. Codes survive reboots without needing a re-sync as long as the RTC drift is acceptable.

Add accounts via the **TOTP** tab (name + base32 secret from the service's 2FA setup page).

---

## Storage (LittleFS)

```
/config/          — badge name, status, tagline, qr_data, spam_ssids,
                    portal_autostart, device_settings, troll_ssid,
                    portal_password (SHA-256 hash), totp_accounts
/ir/saved/        — captured IR signals (0001.ir + 0001.txt pairs)
/ir/uploads/      — Flipper Zero .ir files
/web/             — portal web UI (index.html, app.js, style.css, troll.html)
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
src/storage/      LittleFS driver + settings/IR/TOTP file management
src/ir/           IRremoteESP8266 capture/replay + Flipper format parser/serializer
src/led/          LED animation state machine
src/portal/       WebServer + DNSServer + REST API + session auth
src/wifi/         WifiSpammer (beacon flood), WifiDeauth, WifiKarma, TrollPortal (karma AP)
```

All managers are passed to every app via `AppContext` (service locator). Apps are singletons instantiated once at boot; `AppManager::switchTo()` calls `onExit()` / `onEnter()` on transitions.

E-ink rendering uses a **page-loop** pattern — always wrap draw calls between `startFullWindowDraw()` / `startPartialWindowDraw()` and `while (display.nextPage())`.

---

## Adding an App

1. Subclass `IApp`, implement `onEnter`, `onExit`, `handleButton`, `update`, `render`.
2. Access any manager via `_appManager->context()->storage` / `->ir` / `->power` / etc.
3. Instantiate at file scope in `main.cpp`, call `setup()` after other apps, wire into a parent menu.

See any existing app in `src/apps/` for a working example.
