#include "BadgeApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "display/DisplayManager.h"
#include "storage/StorageManager.h"

#include <qrcode.h>

// ─── Font metrics for FreeMonoBold9pt7b ──────────────────────────────────────
// Y coordinate passed to drawText() is the BASELINE.
// Capital ascent above baseline: ~12px. Descender below baseline: ~3px.
//
// For the default (no) font, Y coordinate is the TOP of the character.
// Character height: 8px.  Character width: 6px per glyph.

namespace {
    constexpr uint8_t DISP_W    = 250;  // display width  (rotation=3)
    constexpr uint8_t DISP_H    = 122;  // display height (rotation=3)

    constexpr int BAR_H         = 20;   // status bar occupies y 0-19
    constexpr int MARGIN_X      = 5;

    // ── Text mode ─────────────────────────────────────
    constexpr int TM_NAME_BL    = 36;   // title font baseline (cap top ≈ y=24)
    constexpr int TM_SEP_Y      = 42;   // 1px separator line
    constexpr int TM_BOX_Y      = 47;   // status pill top
    constexpr int TM_BOX_H      = 12;   // status pill height
    constexpr int TM_STAT_TY    = 48;   // default font TOP inside pill
    constexpr int TM_TAG_TY     = 66;   // tagline default font TOP
    constexpr int TM_DOT_Y      = 86;   // mode dot top
    constexpr int TM_HINT_TY    = 110;  // hint default font TOP

    // ── QR + Side mode ────────────────────────────────
    constexpr int QS_QR_X       = 3;    // QR left edge
    constexpr int QS_QR_Y       = 22;   // QR top (just below divider)
    constexpr int QS_QR_GAP     = 6;    // gap between QR right edge and text
    constexpr int QS_QR_SCALE   = 2;    // pixels per QR module
    constexpr int QS_NAME_BL    = 36;   // title font baseline (same as text mode)
    constexpr int QS_STAT_TY    = 44;   // status default font TOP
    constexpr int QS_TAG_TY     = 57;   // tagline default font TOP
    constexpr int QS_DOT_Y      = 72;   // mode dot top
    constexpr int QS_HINT_TY    = 110;  // hint TOP

    // ── Mode dots ─────────────────────────────────────
    constexpr int DOT_SIZE      = 6;
    constexpr int DOT_SPACING   = 10;

    // QR version cap
    constexpr uint8_t MAX_QR_VER = 4;
}

BadgeApp::BadgeApp() {}

void BadgeApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp  = returnApp;
}

void BadgeApp::onEnter() {
    Serial.println("[BadgeApp] enter");
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    if (ctx && ctx->storage) {
        int m = ctx->storage->getBadgeMode();
        if (m < 0 || m >= (int)Mode::COUNT) m = 0;
        _mode = (Mode)m;
    }
    _needsRender = true;
}

void BadgeApp::onExit() {
    Serial.println("[BadgeApp] exit");
}

void BadgeApp::handleButton(const ButtonEvent& event) {
    if (!_appManager || event.action != ButtonAction::Press) return;

    if (event.id == ButtonId::Back) {
        if (_returnApp) _appManager->switchTo(_returnApp);
        return;
    }

    if (event.id == ButtonId::Select) {
        int next = ((int)_mode + 1) % (int)Mode::COUNT;
        _mode = (Mode)next;
        auto* ctx = _appManager->context();
        if (ctx && ctx->storage) ctx->storage->setBadgeMode(next);
        _needsRender = true;
    }
}

void BadgeApp::update() {}

void BadgeApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    _needsRender = false;

    switch (_mode) {
        case Mode::Text:   renderText(display);   break;
        case Mode::QrSide: renderQrSide(display); break;
        case Mode::QrFull: renderQrFull(display); break;
        default:           renderText(display);   break;
    }
}

// ─── Mode dots helper (used in Text and QrSide) ───────────────────────────────

static void drawModeDots(DisplayManager& display, int x0, int y, int currentMode) {
    for (int i = 0; i < (int)BadgeApp::MODE_COUNT; i++) {
        int bx = x0 + i * DOT_SPACING;
        if (i == currentMode) display.fillRect(bx, y, DOT_SIZE, DOT_SIZE);
        else                   display.drawRect(bx, y, DOT_SIZE, DOT_SIZE);
    }
}

// ─── QR helper (compute + draw inside page loop) ─────────────────────────────

// initQr: tries versions 1-MAX_QR_VER at ECC_LOW.
// Returns true and fills qrcode / buf.
// Non-const per ricmoo library API
static bool initQr(QRCode& qrcode, uint8_t* buf, const String& data) {
    if (data.isEmpty()) return false;
    for (uint8_t v = 1; v <= MAX_QR_VER; v++) {
        if (qrcode_initText(&qrcode, buf, v, ECC_LOW, data.c_str()))
            return true;
    }
    return false;
}

static void drawQrModules(DisplayManager& display,
                           QRCode& qrcode, int ox, int oy, int scale) {
    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                display.fillRect(ox + qx * scale, oy + qy * scale, scale, scale);
            }
        }
    }
}

// ─── Mode 0 – Text ───────────────────────────────────────────────────────────

void BadgeApp::renderText(DisplayManager& display) {
    auto* stor = (_appManager && _appManager->context())
                 ? _appManager->context()->storage : nullptr;

    String name    = stor ? stor->getBadgeName()    : "Stealthy";
    String status  = stor ? stor->getBadgeStatus()  : "Online";
    String tagline = stor ? stor->getBadgeTagline() : "";

    // Status pill width: 6px per char + 10px padding, min 40
    int pillW = max(40, (int)status.length() * 6 + 10);

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        display.setTextBlack();

        // Name (title font, Y=baseline)
        display.setTitleFont();
        display.drawText(MARGIN_X, TM_NAME_BL, name.c_str());

        // Separator under name
        display.fillRect(0, TM_SEP_Y, DISP_W, 1);

        // Status pill (default font, Y=top of char)
        display.setDefaultFont();
        display.drawRect(MARGIN_X, TM_BOX_Y, pillW, TM_BOX_H);
        display.drawText(MARGIN_X + 4, TM_STAT_TY, status.c_str());

        // Tagline (default font, Y=top)
        if (!tagline.isEmpty()) {
            display.drawText(MARGIN_X, TM_TAG_TY, tagline.c_str());
        }

        // Mode dots
        drawModeDots(display, MARGIN_X, TM_DOT_Y, (int)_mode);

        // Hint
        display.drawText(MARGIN_X, TM_HINT_TY, "Sel:view  Back:exit");
    } while (display.nextPage());
}

// ─── Mode 1 – QR + info side-by-side ─────────────────────────────────────────

void BadgeApp::renderQrSide(DisplayManager& display) {
    auto* stor = (_appManager && _appManager->context())
                 ? _appManager->context()->storage : nullptr;

    String name    = stor ? stor->getBadgeName()    : "Stealthy";
    String status  = stor ? stor->getBadgeStatus()  : "Online";
    String tagline = stor ? stor->getBadgeTagline() : "";
    String qrData  = stor ? stor->getBadgeQrData()  : "";

    // Build QR once before page loop
    QRCode  qrcode;
    uint8_t buf[qrcode_getBufferSize(MAX_QR_VER)];
    bool    hasQr = initQr(qrcode, buf, qrData);

    int qrPx  = hasQr ? (int)qrcode.size * QS_QR_SCALE : 0;
    int textX = QS_QR_X + qrPx + QS_QR_GAP;

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        display.setTextBlack();

        if (hasQr) {
            drawQrModules(display, qrcode, QS_QR_X, QS_QR_Y, QS_QR_SCALE);
        } else {
            // Placeholder when no QR data
            display.setDefaultFont();
            display.drawRect(QS_QR_X, QS_QR_Y, 46, 46);
            display.drawText(QS_QR_X + 5, QS_QR_Y + 14, "No QR");
            display.drawText(QS_QR_X + 5, QS_QR_Y + 26, "Set in");
            display.drawText(QS_QR_X + 5, QS_QR_Y + 36, "portal");
        }

        // Name (title font, Y=baseline)
        display.setTitleFont();
        display.drawText(textX, QS_NAME_BL, name.c_str());

        // Status (default font, Y=top)
        display.setDefaultFont();
        display.drawText(textX, QS_STAT_TY, ("[ " + status + " ]").c_str());

        // Tagline
        if (!tagline.isEmpty()) {
            display.drawText(textX, QS_TAG_TY, tagline.c_str());
        }

        // Mode dots (right column)
        drawModeDots(display, textX, QS_DOT_Y, (int)_mode);

        // Hint (full width, bottom)
        display.drawText(MARGIN_X, QS_HINT_TY, "Sel:view  Back:exit");
    } while (display.nextPage());
}

// ─── Mode 2 – Full-screen QR ─────────────────────────────────────────────────

void BadgeApp::renderQrFull(DisplayManager& display) {
    auto* stor = (_appManager && _appManager->context())
                 ? _appManager->context()->storage : nullptr;

    String qrData = stor ? stor->getBadgeQrData() : "";
    String name   = stor ? stor->getBadgeName()   : "Stealthy";

    QRCode  qrcode;
    uint8_t buf[qrcode_getBufferSize(MAX_QR_VER)];
    bool    hasQr = initQr(qrcode, buf, qrData);

    int scale = 1;
    int ox = 0, oy = 0;

    if (hasQr && qrcode.size > 0) {
        // Auto-scale: fill height (leave 2px margin)
        scale = (DISP_H - 4) / qrcode.size;
        if (scale < 1) scale = 1;
        if (scale > 3) scale = 3;  // cap so it doesn't look too blocky

        int side = (int)qrcode.size * scale;
        ox = (DISP_W - side) / 2;
        oy = (DISP_H - side) / 2;
    }

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.setTextBlack();

        if (hasQr) {
            drawQrModules(display, qrcode, ox, oy, scale);
        } else {
            display.setDefaultFont();
            display.drawText(8, 48, "No QR data set.");
            display.drawText(8, 60, "Open portal, Badge tab,");
            display.drawText(8, 70, "enter QR Code Data.");
            display.drawText(8, 82, "Sel:cycle  Back:exit");
        }
    } while (display.nextPage());
}
