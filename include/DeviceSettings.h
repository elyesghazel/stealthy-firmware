#pragma once

struct DeviceSettings {
    int sleepTimeoutIndex = 1;      // 0=10s, 1=30s, 2=1min, 3=3min
    int refreshIntervalIndex = 1;   // 0=10, 1=25, 2=50
    int badgeStatusIndex = 0;       // 0=Online, 1=Busy, 2=Away, 3=Offline
    bool showStartScreen = true;
};