#include "RecoveryBoot.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <esp_system.h>

#include "Pins.h"
#include "display/DisplayManager.h"

namespace {

constexpr unsigned long ARM_HOLD_MS = 2000;
constexpr int BTN_ACTIVE = LOW;

bool pressed(int pin) {
    return digitalRead(pin) == BTN_ACTIVE;
}

bool armCombo() {
    return pressed(Pins::BTN_UP) && pressed(Pins::BTN_BACK);
}

void drawScreen(DisplayManager& display, const char* heading, const char* line1, const char* line2) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.setTextBlack();
        display.setTitleFont();
        display.drawText(8, 24, heading);
        display.setDefaultFont();
        display.drawText(8, 52, line1);
        display.drawText(8, 72, line2);
    } while (display.nextPage());
}

// Recursively delete everything under `path`. Returns the count of files removed.
int wipeDir(const char* path) {
    int removed = 0;
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) return 0;

    File entry = dir.openNextFile();
    while (entry) {
        // Arduino LittleFS sometimes returns a full path, sometimes just the
        // leaf. Treat an absolute name as already-rooted.
        String name = String(entry.name());
        String full;
        if (name.startsWith("/")) {
            full = name;
        } else {
            full = String(path);
            if (!full.endsWith("/")) full += "/";
            full += name;
        }

        bool isDir = entry.isDirectory();
        entry.close();

        if (isDir) {
            removed += wipeDir(full.c_str());
            LittleFS.rmdir(full.c_str());
        } else if (LittleFS.remove(full.c_str())) {
            removed += 1;
        }
        entry = dir.openNextFile();
    }
    dir.close();
    return removed;
}

}

void RecoveryBoot::check(DisplayManager& display) {
    // Gate: the combo has to be held already at boot. No prompt otherwise.
    if (!armCombo()) return;

    drawScreen(display, "Recovery", "Keep Up+Back held", "to factory reset.");

    unsigned long start = millis();
    while (armCombo()) {
        if (millis() - start >= ARM_HOLD_MS) break;
        delay(20);
    }

    if (!armCombo()) {
        // Released early, carry on booting.
        return;
    }

    drawScreen(display, "Factory reset?", "Select: wipe /config", "Back:   cancel");

    // Wait for a decision. Debounce with a short settle on release so we
    // don't catch the same press that got us here.
    bool wantReset = false;
    while (true) {
        bool sel = pressed(Pins::BTN_SELECT);
        bool back = pressed(Pins::BTN_BACK);

        if (sel) { wantReset = true; break; }
        if (back && !armCombo()) { wantReset = false; break; }

        delay(20);
    }

    // Wait for release so the next boot doesn't re-arm.
    while (pressed(Pins::BTN_SELECT) || pressed(Pins::BTN_BACK) || pressed(Pins::BTN_UP)) {
        delay(20);
    }

    if (!wantReset) {
        drawScreen(display, "Cancelled", "Normal boot", "resuming...");
        delay(600);
        return;
    }

    // Mount LittleFS if the storage manager hasn't run yet.
    if (!LittleFS.begin(true)) {
        drawScreen(display, "Reset failed", "Filesystem mount", "failed. Reboot.");
        delay(1500);
        esp_restart();
    }

    int removed = wipeDir("/config");
    LittleFS.rmdir("/config");

    char line[32];
    snprintf(line, sizeof(line), "%d files removed", removed);
    drawScreen(display, "Done", line, "Rebooting...");
    delay(1200);

    esp_restart();
}
