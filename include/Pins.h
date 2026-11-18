#pragma once

#include <driver/gpio.h>
#include <stdint.h>

// All GPIO assignments for the Stealthy badge in one place.
// 4D Systems Gen4 R8N16 (ESP32-S3).
namespace Pins {

// Buttons (active-low with internal pull-ups).
constexpr int BTN_UP     = 10;
constexpr int BTN_DOWN   = 11;
constexpr int BTN_SELECT = 12;
constexpr int BTN_BACK   = 13;

// Status LEDs.
constexpr int LED_BLUE  = 2;
constexpr int LED_GREEN = 1;

// Infrared.
constexpr int IR_TX        = 14;
constexpr int IR_RX        = 21;
constexpr int IR_STATUS_LED = LED_BLUE;

// Power.
constexpr int BATTERY_ADC = 9;

// Wake source for deep sleep. Must be an RTC GPIO; shares the Select button.
constexpr gpio_num_t WAKE = GPIO_NUM_12;

}
