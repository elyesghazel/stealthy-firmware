# Graph Report - .  (2026-06-08)

## Corpus Check
- Corpus is ~31,111 words - fits in a single context window. You may not need a graph.

## Summary
- 731 nodes · 1209 edges · 61 communities (40 shown, 21 thin omitted)
- Extraction: 99% EXTRACTED · 1% INFERRED · 0% AMBIGUOUS · INFERRED: 17 edges (avg confidence: 0.91)
- Token cost: 7,200 input · 3,800 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Portal & IR Tools Core|Portal & IR Tools Core]]
- [[_COMMUNITY_IR Storage & File Management|IR Storage & File Management]]
- [[_COMMUNITY_Web Portal Frontend|Web Portal Frontend]]
- [[_COMMUNITY_IR Tools UI|IR Tools UI]]
- [[_COMMUNITY_Badge App & Service Locator|Badge App & Service Locator]]
- [[_COMMUNITY_Settings UI|Settings UI]]
- [[_COMMUNITY_IR Capture & Flipper Format|IR Capture & Flipper Format]]
- [[_COMMUNITY_E-ink Display Layer|E-ink Display Layer]]
- [[_COMMUNITY_Main Menu Navigation|Main Menu Navigation]]
- [[_COMMUNITY_TOTP Authenticator UI|TOTP Authenticator UI]]
- [[_COMMUNITY_WiFi Deauth App|WiFi Deauth App]]
- [[_COMMUNITY_Boot & Start Screen|Boot & Start Screen]]
- [[_COMMUNITY_WiFi Tools Menu|WiFi Tools Menu]]
- [[_COMMUNITY_Portal App UI|Portal App UI]]
- [[_COMMUNITY_Beacon Flood App|Beacon Flood App]]
- [[_COMMUNITY_Architecture Docs|Architecture Docs]]
- [[_COMMUNITY_Karma AP & Probing|Karma AP & Probing]]
- [[_COMMUNITY_TOTP Backend|TOTP Backend]]
- [[_COMMUNITY_Button Input System|Button Input System]]
- [[_COMMUNITY_LittleFS Storage Driver|LittleFS Storage Driver]]
- [[_COMMUNITY_About App|About App]]
- [[_COMMUNITY_LED Animation Manager|LED Animation Manager]]
- [[_COMMUNITY_WiFi Spammer Backend|WiFi Spammer Backend]]
- [[_COMMUNITY_Flipper IR Parser|Flipper IR Parser]]
- [[_COMMUNITY_IR Hardware Driver|IR Hardware Driver]]
- [[_COMMUNITY_WiFi Deauth Engine|WiFi Deauth Engine]]
- [[_COMMUNITY_Config & Storage Docs|Config & Storage Docs]]
- [[_COMMUNITY_Badge Config Files|Badge Config Files]]
- [[_COMMUNITY_Device Settings & Portal Config|Device Settings & Portal Config]]
- [[_COMMUNITY_Power & Battery Manager|Power & Battery Manager]]
- [[_COMMUNITY_Flipper IR Serializer|Flipper IR Serializer]]
- [[_COMMUNITY_WiFi Attack Documentation|WiFi Attack Documentation]]
- [[_COMMUNITY_LED GPIO Driver|LED GPIO Driver]]
- [[_COMMUNITY_App Framework Docs|App Framework Docs]]
- [[_COMMUNITY_Portal REST API Docs|Portal REST API Docs]]
- [[_COMMUNITY_Main Entry & App Context|Main Entry & App Context]]
- [[_COMMUNITY_WiFi Deauth App Header|WiFi Deauth App Header]]
- [[_COMMUNITY_WiFi Spammer App Header|WiFi Spammer App Header]]
- [[_COMMUNITY_WiFi Tools App Header|WiFi Tools App Header]]
- [[_COMMUNITY_Display Manager Header|Display Manager Header]]
- [[_COMMUNITY_IApp Interface|IApp Interface]]
- [[_COMMUNITY_App Info Namespace|App Info Namespace]]
- [[_COMMUNITY_Power Manager Header|Power Manager Header]]
- [[_COMMUNITY_About App Header|About App Header]]
- [[_COMMUNITY_Badge App Header|Badge App Header]]
- [[_COMMUNITY_Start Screen Header|Start Screen Header]]
- [[_COMMUNITY_Button Manager Header|Button Manager Header]]
- [[_COMMUNITY_IR Driver Header|IR Driver Header]]
- [[_COMMUNITY_LED Driver Header|LED Driver Header]]
- [[_COMMUNITY_LED Manager Header|LED Manager Header]]
- [[_COMMUNITY_Filesystem Driver Header|Filesystem Driver Header]]
- [[_COMMUNITY_TOTP Manager Header|TOTP Manager Header]]
- [[_COMMUNITY_WiFi Deauth Header|WiFi Deauth Header]]
- [[_COMMUNITY_WiFi Karma Header|WiFi Karma Header]]
- [[_COMMUNITY_WiFi Spammer Header|WiFi Spammer Header]]
- [[_COMMUNITY_VS Code Extensions|VS Code Extensions]]

## God Nodes (most connected - your core abstractions)
1. `setupRoutes()` - 39 edges
2. `String` - 36 edges
3. `api()` - 13 edges
4. `AppContext` - 10 edges
5. `DisplayManager` - 9 edges
6. `DisplayManager` - 9 edges
7. `Portal Web UI (index.html — tabbed SPA dashboard)` - 9 edges
8. `requestFullRender()` - 8 edges
9. `moveUp()` - 8 edges
10. `moveDown()` - 8 edges

## Surprising Connections (you probably didn't know these)
- `Main Loop Order (main.cpp)` --references--> `AppManager State Machine`  [EXTRACTED]
  CLAUDE.md → README.md
- `Service Locator Pattern (AppContext)` --rationale_for--> `AppContext Service Locator`  [EXTRACTED]
  CLAUDE.md → README.md
- `LittleFS Storage Layout (/config, /ir, /payloads)` --references--> `StorageManager (LittleFS driver + settings/IR files)`  [EXTRACTED]
  CLAUDE.md → README.md
- `Portal Tab: IR Library (send/rename/export/delete captures)` --references--> `IrManager (capture/replay + Flipper format parser)`  [INFERRED]
  data/web/index.html → README.md
- `Main Loop Order (main.cpp)` --references--> `LedManager (LED animation state machine)`  [EXTRACTED]
  CLAUDE.md → README.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **AppContext bundles all managers and injects them into every IApp** — readme_appcontext, readme_appmanager, readme_iapp_interface, readme_displaymanager, readme_buttonmanager, readme_powermanager, readme_storagemanager, readme_irmanager, readme_ledmanager, readme_portalmanager [EXTRACTED 1.00]
- **Portal Web UI tab system collectively implements the remote control dashboard** — web_index_html, web_tab_system, web_tab_badge, web_tab_ir_lib, web_tab_ir_upload, web_tab_flood, web_tab_totp, web_tab_recon [EXTRACTED 1.00]
- **LittleFS config directory stores all persistent device settings as flat text files** — readme_littlefs, claude_md_storage_layout, config_name, config_status, config_badge_mode, config_device_settings, config_portal_autostart, config_qr_data, config_spam_ssids, config_tagline, config_totp_accounts [EXTRACTED 1.00]

## Communities (61 total, 21 thin omitted)

### Community 0 - "Portal & IR Tools Core"
Cohesion: 0.06
Nodes (53): IrManager, begin(), contentTypeForPath(), handleApiBattery(), handleApiFileDelete(), handleApiFileDownload(), handleApiFileUpload(), handleApiFsList() (+45 more)

### Community 1 - "IR Storage & File Management"
Cohesion: 0.08
Nodes (53): IrSavedItem, IrUploadItem, IrUploadSignal, decode_type_t, DeviceSettings, FileEntry, IrCapture, String (+45 more)

### Community 2 - "Web Portal Frontend"
Cohesion: 0.10
Nodes (37): api(), apiPost(), base32Decode(), buildFileRow(), buildIrRow(), computeTotp(), dropZone, irDelete() (+29 more)

### Community 3 - "IR Tools UI"
Cohesion: 0.17
Nodes (30): clampMenuScroll(), clampSavedScroll(), clampSignalScroll(), clampUploadScroll(), drawCapturedInfo(), drawMenu(), drawSavedList(), drawUploadList() (+22 more)

### Community 4 - "Badge App & Service Locator"
Cohesion: 0.09
Nodes (22): AppContext, drawLockBitmap(), handleButton(), render(), setup(), AppManager(), begin(), context() (+14 more)

### Community 5 - "Settings UI"
Cohesion: 0.15
Nodes (25): applyAndSave(), clampScrollToSelection(), currentValueText(), drawScrollIndicators(), drawSettingsList(), drawValue(), goBack(), handleButton() (+17 more)

### Community 6 - "IR Capture & Flipper Format"
Cohesion: 0.08
Nodes (14): FileSystemDriver, FlipperIrParser(), FlipperIrSerializer(), setLastCapture(), IrDriver, class, class, IrManager() (+6 more)

### Community 7 - "E-ink Display Layer"
Cohesion: 0.13
Nodes (16): attachPowerManager(), batteryBarsFromVoltage(), begin(), drawBatteryIcon(), drawRect(), drawStatusBar(), drawText(), fillRect() (+8 more)

### Community 8 - "Main Menu Navigation"
Cohesion: 0.18
Nodes (20): clampScrollToSelection(), drawMenuItems(), drawRow(), drawScrollIndicators(), handleButton(), moveSelectionDown(), moveSelectionUp(), onEnter() (+12 more)

### Community 9 - "TOTP Authenticator UI"
Cohesion: 0.18
Nodes (20): boxX(), drawContent(), drawDigitBoxes(), drawNoAccounts(), drawNoTimeSync(), drawProgressBar(), handleButton(), onEnter() (+12 more)

### Community 10 - "WiFi Deauth App"
Cohesion: 0.22
Nodes (17): clampScroll(), drawApList(), drawContent(), handleButton(), onEnter(), render(), renderFull(), renderPartial() (+9 more)

### Community 11 - "Boot & Start Screen"
Cohesion: 0.20
Nodes (15): drawBootContent(), drawProgressBar(), handleButton(), onEnter(), render(), renderFull(), renderPartial(), requestFullRender() (+7 more)

### Community 12 - "WiFi Tools Menu"
Cohesion: 0.22
Nodes (15): drawMenu(), handleButton(), onEnter(), render(), renderFull(), renderPartial(), requestFullRender(), requestPartialRender() (+7 more)

### Community 13 - "Portal App UI"
Cohesion: 0.21
Nodes (13): drawContent(), handleButton(), onEnter(), render(), renderFull(), renderPartial(), requestFullRender(), requestPartialRender() (+5 more)

### Community 14 - "Beacon Flood App"
Cohesion: 0.23
Nodes (13): drawContent(), handleButton(), onEnter(), render(), renderFull(), renderPartial(), requestFullRender(), requestPartialRender() (+5 more)

### Community 15 - "Architecture Docs"
Cohesion: 0.18
Nodes (14): ButtonManager Circular Event Queue, Deep Sleep (EXT1 GPIO wakeup, 5-min default timeout), Main Loop Order (main.cpp), Partial E-ink Update (minimize full-screen flicker), Service Locator Pattern (AppContext), PortalApp, AppContext Service Locator, ButtonManager (debounce, long-press, event queue) (+6 more)

### Community 16 - "Karma AP & Probing"
Cohesion: 0.16
Nodes (8): ProbeEntry, String, vector, wifi_promiscuous_pkt_type_t, getProbes(), promiscCallback(), startKarma(), stopSniff()

### Community 17 - "TOTP Backend"
Cohesion: 0.29
Nodes (13): String, addAccount(), base32Decode(), begin(), computeTotp(), currentUnixTime(), deleteAccount(), generateCode() (+5 more)

### Community 18 - "Button Input System"
Cohesion: 0.22
Nodes (10): ButtonAction, ButtonId, ButtonState, getNextEvent(), hasEvent(), pushEvent(), readPressed(), update() (+2 more)

### Community 19 - "LittleFS Storage Driver"
Cohesion: 0.22
Nodes (9): FileEntry, String, vector, ensureDir(), exists(), listEntries(), listFiles(), readTextFile() (+1 more)

### Community 20 - "About App"
Cohesion: 0.21
Nodes (7): handleButton(), render(), setup(), AppManager, ButtonEvent, DisplayManager, IApp

### Community 21 - "LED Animation Manager"
Cohesion: 0.26
Nodes (8): enterMode(), showBatteryLow(), showBooting(), showIrTransmit(), showSuccess(), LedDriver, Mode, LedManager()

### Community 22 - "WiFi Spammer Backend"
Cohesion: 0.23
Nodes (7): String, vector, buildBssid(), runLoop(), sendBeacon(), setSSIDs(), spamTask()

### Community 23 - "Flipper IR Parser"
Cohesion: 0.36
Nodes (10): FlipperSignal, hexBytes(), mapProto(), parse(), parseParsed(), parseRaw(), decode_type_t, IrCapture (+2 more)

### Community 24 - "IR Hardware Driver"
Cohesion: 0.22
Nodes (6): protocolToString(), readCapture(), send(), decode_type_t, IrCapture, String

### Community 25 - "WiFi Deauth Engine"
Cohesion: 0.24
Nodes (5): wifi_promiscuous_pkt_type_t, deauthTask(), promiscCallback(), runLoop(), sendDeauthFrame()

### Community 26 - "Config & Storage Docs"
Cohesion: 0.24
Nodes (10): LittleFS Storage Layout (/config, /ir, /payloads), FileSystemDriver (LittleFS wrapper with listEntries/ensureDir), StorageManager Extensions (listFsEntries, ensureDirectory), Payload: demo.txt (BadUSB-style DELAY/STRING/ENTER script), E-ink Display (GxEPD2, 2.13", 250x122px), ESP32-S3 MCU (4D Systems Gen4 R8N16), LittleFS Filesystem, PlatformIO Build System (+2 more)

### Community 27 - "Badge Config Files"
Cohesion: 0.22
Nodes (10): Config: badge_mode.txt, Config: device name ("elyes"), Config: qr_data.txt, Config: badge status ("Stealthy Badge"), Config: tagline.txt, Config: totp_accounts.txt (TOTP account secrets), BadgeApp, TOTP Authenticator (offline 2FA, e-ink display) (+2 more)

### Community 28 - "Device Settings & Portal Config"
Cohesion: 0.24
Nodes (10): Config: device_settings.txt, Config: portal_autostart.txt, IrToolsApp, Flipper Zero .ir File Format, IrManager (capture/replay + Flipper format parser), Portal Web UI (index.html — tabbed SPA dashboard), Portal Tab: IR Library (send/rename/export/delete captures), Portal Tab: IR Upload (Flipper .ir file drag-and-drop) (+2 more)

### Community 30 - "Flipper IR Serializer"
Cohesion: 0.58
Nodes (8): decodeValue(), getFlipperProto(), hexBytesStr(), serializeFile(), serializeSignal(), IrCapture, String, vector

### Community 31 - "WiFi Attack Documentation"
Cohesion: 0.32
Nodes (8): Config: spam_ssids.txt (beacon flood SSID list), Beacon Flood (fake SSID broadcast), WiFi Deauth (802.11 deauthentication frames), Karma AP (rogue AP cloning probed SSIDs), Probe Sniffer (passive 802.11 probe request capture), WiFi Tools (WifiSpammer, WifiDeauth, WifiKarma), Portal Tab: Flood (beacon flood SSID list editor), Portal Tab: Recon (probe sniffer + Karma AP control)

### Community 32 - "LED GPIO Driver"
Cohesion: 0.39
Nodes (4): begin(), setAllOff(), setBlue(), setGreen()

### Community 33 - "App Framework Docs"
Cohesion: 0.33
Nodes (6): Hierarchical App State Machine, MainMenuApp, SettingsApp, WiFiToolsApp (Beacon Flood + Deauth), AppManager State Machine, IApp Interface

### Community 34 - "Portal REST API Docs"
Cohesion: 0.47
Nodes (6): REST API: /api/file/download, REST API: /api/file/upload, REST API: /api/fs/list, IR File Browser Frontend (portal JS/HTML for LittleFS navigation), PortalManager Extensions (fs list/download/upload API endpoints), isSafeFsPath() Path Validation Helper

### Community 35 - "Main Entry & App Context"
Cohesion: 0.47
Nodes (3): initFileSystem(), printResetReason(), setup()

## Knowledge Gaps
- **98 isolated node(s):** `recommendations`, `unwantedRecommendations`, `TAB_LOADERS`, `dropZone`, `uploadInput` (+93 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **21 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `IrDriver` connect `IR Capture & Flipper Format` to `Portal & IR Tools Core`, `IR Tools UI`?**
  _High betweenness centrality (0.091) - this node is a cross-community bridge._
- **Why does `AppContext` connect `Badge App & Service Locator` to `IR Tools UI`, `TOTP Authenticator UI`, `WiFi Deauth App`, `Boot & Start Screen`, `Portal App UI`, `Beacon Flood App`?**
  _High betweenness centrality (0.085) - this node is a cross-community bridge._
- **What connects `recommendations`, `unwantedRecommendations`, `TAB_LOADERS` to the rest of the system?**
  _102 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Portal & IR Tools Core` be split into smaller, more focused modules?**
  _Cohesion score 0.06292966684294024 - nodes in this community are weakly interconnected._
- **Should `IR Storage & File Management` be split into smaller, more focused modules?**
  _Cohesion score 0.07615018508725542 - nodes in this community are weakly interconnected._
- **Should `Web Portal Frontend` be split into smaller, more focused modules?**
  _Cohesion score 0.10384615384615385 - nodes in this community are weakly interconnected._
- **Should `Badge App & Service Locator` be split into smaller, more focused modules?**
  _Cohesion score 0.09425287356321839 - nodes in this community are weakly interconnected._