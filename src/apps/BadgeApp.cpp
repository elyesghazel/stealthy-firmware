#include "BadgeApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "display/DisplayManager.h"
#include "storage/StorageManager.h"
#include "power/PowerManager.h"

#include <qrcode.h>

// ─── Display geometry ──────────────────────────────────────────────────────────
// GxEPD2_213_B74 at rotation=3 → 250 × 122 px (landscape)
//
// Status bar: y=0..19 (drawStatusBar() fills this)
// Content:    y=20..121  (102 px tall)
//
// Layout:
//   Left column  x=0..153   (154 px) — identity info
//   Divider      x=154      (1 px)
//   Right column x=156..249 (94 px)  — QR code or lock bitmap
//
// Font notes:
//   Title font (FreeMonoBold9pt7b): Y argument = BASELINE
//     cap ascent ≈ 12 px above baseline, descent ≈ 3 px below
//   Default font: Y argument = TOP of character, char height = 8 px, width ≈ 6 px
// ──────────────────────────────────────────────────────────────────────────────
namespace {
    // Column geometry
    constexpr int MX      =  4;    // left margin x
    constexpr int LEFT_W  = 153;   // left column width
    constexpr int DIV_X   = 154;   // divider x
    constexpr int RIG_X   = 156;   // right column start x
    constexpr int RIG_W   =  94;   // 250 - 156
    constexpr int RIG_CX  = RIG_X + RIG_W / 2;   // 156 + 47 = 203
    constexpr int CONT_Y  =  20;   // content top y (below status bar)
    constexpr int CONT_H  = 102;   // content height (122 - 20)
    constexpr int RIG_CY  = CONT_Y + CONT_H / 2;  // 20 + 51 = 71

    // ── Left column vertical positions (default font Y = top of char, 8px tall)
    //  y=20: content starts (below status bar divider)
    //  y=24: name cap-top  (NAME_BL - 12)
    //  y=36: name baseline (title font)
    //  y=41: horizontal rule (1 px)
    //  y=47: tagline top
    //  y=59: battery top
    //  y=71: IR captures top
    //  y=83: WiFi SSIDs top
    //  y=95: uptime top
    //  y=113: hint top  (bottom at y=120, 1 px from screen edge)
    constexpr int NAME_BL  = 36;
    constexpr int RULE_Y   = 41;
    constexpr int TAG_TY   = 47;
    constexpr int BAT_TY   = 59;
    constexpr int IR_TY    = 71;
    constexpr int WIFI_TY  = 83;
    constexpr int UP_TY    = 95;
    constexpr int HINT_TY  = 113;

    // QR — buffer MUST be a compile-time constant, not a VLA.
    // qrcode_getBufferSize(v) = ((v*4+17)^2 + 7) / 8
    // v=1→56, v=2→79, v=3→106, v=4→137  (137, not 136!)
    // qrcode_initText() returns 0 on success, negative on failure (C-style).
    constexpr uint8_t MAX_VER   = 4;
    constexpr int     QR_BUF_SZ = 137;
    constexpr int     QR_SCALE  = 3;
}

// ─────────────────────────────────────────────────────────────────────────────

BadgeApp::BadgeApp() {}

void BadgeApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp  = returnApp;
}

void BadgeApp::onEnter() {
    Serial.println("[BadgeApp] enter");
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    if (ctx && ctx->storage) {
        _irCount   = (int)ctx->storage->listIrCaptures().size();
        _ssidCount = (int)ctx->storage->loadSpamSSIDs().size();
    } else {
        _irCount = _ssidCount = 0;
    }
    _needsRender = true;
}

void BadgeApp::onExit() {
    Serial.println("[BadgeApp] exit");
}

void BadgeApp::handleButton(const ButtonEvent& event) {
    if (!_appManager || event.action != ButtonAction::Press) return;
    if (event.id == ButtonId::Back && _returnApp)
        _appManager->switchTo(_returnApp);
}

void BadgeApp::update() {}

// ─── Render ──────────────────────────────────────────────────────────────────

void BadgeApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    _needsRender = false;

    auto* ctx  = _appManager ? _appManager->context() : nullptr;
    auto* stor = ctx ? ctx->storage : nullptr;

    String name    = stor ? stor->getBadgeName()    : "Stealthy";
    String tagline = stor ? stor->getBadgeTagline() : "";
    String qrData  = stor ? stor->getBadgeQrData()  : "";
    float  voltage = (ctx && ctx->power) ? ctx->power->readBatteryVoltage() : 0.0f;
    int    pct     = (ctx && ctx->power) ? ctx->power->batteryPercent()     : 0;

    // Uptime
    unsigned long upMs = millis();
    int upH = (int)(upMs / 3600000UL);
    int upM = (int)((upMs % 3600000UL) / 60000UL);

    // Clamp to fit left column (title font ≈12px/char, default font 6px/char)
    if (name.length()    > 12) name    = name.substring(0, 12);
    if (tagline.length() > 25) tagline = tagline.substring(0, 25);

    // ── Build QR once, before the page loop ──────────────────────────────────
    QRCode  qrcode;
    uint8_t buf[QR_BUF_SZ];   // compile-time constant — no VLA
    bool    hasQr = false;

    if (!qrData.isEmpty()) {
        for (uint8_t v = 1; v <= MAX_VER; v++) {
            // Returns 0 on success, negative on failure (C convention, not bool)
            int8_t result = qrcode_initText(&qrcode, buf, v, ECC_LOW, qrData.c_str());
            Serial.printf("[BadgeApp] QR v%d result=%d size=%d\n",
                          v, result, result == 0 ? qrcode.size : 0);
            if (result == 0) { hasQr = true; break; }
        }
    }
    Serial.printf("[BadgeApp] hasQr=%d\n", hasQr);

    // QR pixel size and centered position inside right column
    int qrPx = hasQr ? (int)qrcode.size * QR_SCALE : 0;
    int qrOx = RIG_X + (RIG_W - qrPx) / 2;
    int qrOy = CONT_Y + (CONT_H - qrPx) / 2;

    // Info line strings
    char batBuf[20];
    char irBuf[20];
    char wifiBuf[20];
    char upBuf[20];
    snprintf(batBuf,  sizeof(batBuf),  "Bat  %.2fV (%d%%)", voltage, pct);
    snprintf(irBuf,   sizeof(irBuf),   "IR   %d cap",       _irCount);
    snprintf(wifiBuf, sizeof(wifiBuf), "WiFi %d SSID",      _ssidCount);
    if (upH > 0) snprintf(upBuf, sizeof(upBuf), "Up   %dh %dm", upH, upM);
    else         snprintf(upBuf, sizeof(upBuf), "Up   %dm",      upM);

    // ── Page loop ─────────────────────────────────────────────────────────────
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        display.setTextBlack();

        // ── Left column ───────────────────────────────────────────────────────

        display.setTitleFont();
        display.drawText(MX, NAME_BL, name.c_str());

        display.setDefaultFont();

        // Thin separator under name
        display.fillRect(0, RULE_Y, LEFT_W, 1);

        // Tagline (omit if empty)
        if (!tagline.isEmpty())
            display.drawText(MX, TAG_TY, tagline.c_str());

        // Device stats
        display.drawText(MX, BAT_TY,  batBuf);
        display.drawText(MX, IR_TY,   irBuf);
        display.drawText(MX, WIFI_TY, wifiBuf);
        display.drawText(MX, UP_TY,   upBuf);

        // Hint at bottom
        display.drawText(MX, HINT_TY, "Back: exit");

        // ── Vertical divider ─────────────────────────────────────────────────
        display.fillRect(DIV_X, CONT_Y, 1, CONT_H);

        // ── Right column ─────────────────────────────────────────────────────
        if (hasQr) {
            for (uint8_t qy = 0; qy < qrcode.size; qy++) {
                for (uint8_t qx = 0; qx < qrcode.size; qx++) {
                    if (qrcode_getModule(&qrcode, qx, qy))
                        display.fillRect(qrOx + qx * QR_SCALE,
                                         qrOy + qy * QR_SCALE,
                                         QR_SCALE, QR_SCALE);
                }
            }
            // "[ scan ]" label below QR if there's room
            int labelY = qrOy + qrPx + 4;
            if (labelY + 8 <= 121) {
                // centre "[ scan ]" (8 chars × 6px = 48px) in 94px column
                display.drawText(RIG_X + (RIG_W - 48) / 2, labelY, "[ scan ]");
            }
        } else {
            drawLockBitmap(display, RIG_CX, RIG_CY);
        }

    } while (display.nextPage());
}

// ─── Padlock bitmap (28 × 38 px) centered at (cx, cy) ────────────────────────
//
//   Shackle (arch):
//     ┌──────────────────┐
//     │  left  │  right  │
//     │  post  │  post   │
//
//   Body (solid black, white interior, keyhole):
//     ┌────────────────────┐
//     │  ┌────────────┐   │
//     │  │   ■■■■     │   │  ← keyhole circle
//     │  │    ■■      │   │  ← keyhole slot
//     │  └────────────┘   │
//     └────────────────────┘
//
void BadgeApp::drawLockBitmap(DisplayManager& display, int cx, int cy) {
    // Top-left anchor so the 28×38 bitmap is centered
    const int ox = cx - 14;
    const int oy = cy - 19;

    // Shackle: left post, right post, top bridge
    display.fillRect(ox +  4, oy,      4, 16);  // left post  (x: ox+4..7,   y: oy..oy+15)
    display.fillRect(ox + 20, oy,      4, 16);  // right post (x: ox+20..23, y: oy..oy+15)
    display.fillRect(ox +  4, oy,     20,  4);  // top bridge (x: ox+4..23,  y: oy..oy+3)

    // Body outer (solid black 28×24, starts at oy+12)
    display.fillRect(ox, oy + 12, 28, 24);
    // Body inner (hollow white, inset 3 px on sides, 3 px from body top)
    display.fillRectWhite(ox + 3, oy + 15, 22, 18);

    // Keyhole: round top (6×5) + slot (4×7)
    display.fillRect(ox + 11, oy + 18, 6, 5);
    display.fillRect(ox + 12, oy + 24, 4, 6);

    // Small label underneath
    // "set QR data" = 11 chars × 6 px = 66 px → start at cx-33 to center
    display.setDefaultFont();
    display.drawText(cx - 33, oy + 41, "set QR data");
}
