Suggested repo layout
stealthy-fw/
├── platformio.ini
├── include/
├── src/
│ ├── main.cpp
│ ├── core/
│ │ ├── AppManager.h
│ │ ├── AppManager.cpp
│ │ ├── ButtonManager.h
│ │ ├── ButtonManager.cpp
│ │ ├── NavigationStack.h
│ │ ├── SettingsManager.h
│ │ └── PowerManager.h
│ ├── drivers/
│ │ ├── DisplayManager.h
│ │ ├── DisplayManager.cpp
│ │ ├── SDCardManager.h
│ │ ├── IRManager.h
│ │ ├── WiFiManager.h
│ │ └── BLEManager.h
│ ├── apps/
│ │ ├── MainMenuApp.h
│ │ ├── MainMenuApp.cpp
│ │ ├── BadgeApp.h
│ │ ├── BadgeApp.cpp
│ │ ├── WiFiToolsApp.h
│ │ ├── BLEToolsApp.h
│ │ ├── IRToolsApp.h
│ │ ├── BadUSBApp.h
│ │ └── SettingsApp.h
│ └── assets/
│ ├── fonts/
│ └── bitmaps/
└── README.md
