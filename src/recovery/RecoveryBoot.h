#pragma once

class DisplayManager;

// Boot-time rescue flow. Call once after the button and display managers are
// up and before loading settings. If Up+Back are held through a short arming
// window, shows a factory-reset screen and waits for the user to confirm with
// Select or cancel with Back. Confirm wipes /config and reboots.
//
// A no-op otherwise, so it's safe to always call from setup().
namespace RecoveryBoot {

void check(DisplayManager& display);

}
