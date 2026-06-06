#include "BadgeApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "display/DisplayManager.h"
#include "storage/StorageManager.h"

#include <qrcode.h>

namespace {
    constexpr int STATUS_BAR_H = 20;   // pixels below status bar divider
    constexpr int HINT_Y       = 114;  // bottom hint line baseline

    // Version 4 buffer fits up to 78 alphanumeric chars at ECC_LOW
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

// ─── QR helper ────────────────────────────────────────────────────────────

uint8_t BadgeApp::drawQrAt(DisplayManager& display,
                            int ox, int oy, int scale, const String& data) {
    if (data.isEmpty() || scale < 1) return 0;

    QRCode  qrcode;
    uint8_t buf[qrcode_getBufferSize(MAX_QR_VER)];

    bool ok = false;
    for (uint8_t v = 1; v <= MAX_QR_VER; v++) {
        if (qrcode_initText(&qrcode, buf, v, ECC_LOW, data.c_str())) {
            ok = true; break;
        }
    }
    if (!ok) return 0;

    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                display.fillRect(ox + qx * scale, oy + qy * scale, scale, scale);
            }
        }
    }
    return qrcode.size;
}

// ─── Mode 0 – Text only ───────────────────────────────────────────────────

void BadgeApp::renderText(DisplayManager& display) {
    auto* stor = (_appManager && _appManager->context())
                 ? _appManager->context()->storage : nullptr;

    String name    = stor ? stor->getBadgeName()    : "Stealthy";
    String status  = stor ? stor->getBadgeStatus()  : "Online";
    String tagline = stor ? stor->getBadgeTagline() : "";

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        display.setTextBlack();

        // Name in title font
        display.setTitleFont();
        display.drawText(5, 40, name.c_str());

        display.setDefaultFont();

        // Status badge
        display.drawRect(4, 46, 8 * (int)status.length() + 6, 14);
        display.drawText(7, 56, status.c_str());

        // Tagline
        if (!tagline.isEmpty()) {
            display.drawText(5, 76, tagline.c_str());
        }

        // Mode indicator dots (filled = current, empty = other)
        for (int i = 0; i < (int)Mode::COUNT; i++) {
            int bx = 5 + i * 12;
            if (i == (int)_mode) display.fillRect(bx, 94, 6, 6);
            else                  display.drawRect(bx, 94, 6, 6);
        }

        display.drawText(5, HINT_Y, "Sel: cycle view   Back: exit");
    } while (display.nextPage());
}

// ─── Mode 1 – QR + info side-by-side ─────────────────────────────────────

void BadgeApp::renderQrSide(DisplayManager& display) {
    auto* stor = (_appManager && _appManager->context())
                 ? _appManager->context()->storage : nullptr;

    String name    = stor ? stor->getBadgeName()    : "Stealthy";
    String status  = stor ? stor->getBadgeStatus()  : "Online";
    String tagline = stor ? stor->getBadgeTagline() : "";
    String qrData  = stor ? stor->getBadgeQrData()  : "";

    // Compute QR size at scale=2 so we know how to lay out the rest
    // (we do this before the page loop; the page loop just re-draws)
    QRCode  qrcode;
    uint8_t buf[qrcode_getBufferSize(MAX_QR_VER)];
    bool    hasQr = false;

    if (!qrData.isEmpty()) {
        for (uint8_t v = 1; v <= MAX_QR_VER; v++) {
            if (qrcode_initText(&qrcode, buf, v, ECC_LOW, qrData.c_str())) {
                hasQr = true; break;
            }
        }
    }

    constexpr int QR_SCALE = 2;
    constexpr int QR_OX    = 3;
    constexpr int QR_OY    = 23;   // just below status bar divider
    int           textX    = hasQr ? (QR_OX + (int)qrcode.size * QR_SCALE + 6) : 5;

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        display.setTextBlack();

        if (hasQr) {
            for (uint8_t qy = 0; qy < qrcode.size; qy++) {
                for (uint8_t qx = 0; qx < qrcode.size; qx++) {
                    if (qrcode_getModule(&qrcode, qx, qy)) {
                        display.fillRect(QR_OX + qx * QR_SCALE,
                                         QR_OY + qy * QR_SCALE,
                                         QR_SCALE, QR_SCALE);
                    }
                }
            }
        } else {
            display.drawText(5, 44, "(no QR)");
            display.drawText(5, 58, "Set in");
            display.drawText(5, 70, "portal");
        }

        // Info column
        display.setTitleFont();
        display.drawText(textX, 38, name.c_str());

        display.setDefaultFont();
        display.drawText(textX, 54, ("[ " + status + " ]").c_str());

        if (!tagline.isEmpty()) {
            display.drawText(textX, 68, tagline.c_str());
        }

        // Mode dots
        for (int i = 0; i < (int)Mode::COUNT; i++) {
            int bx = textX + i * 12;
            if (i == (int)_mode) display.fillRect(bx, 82, 6, 6);
            else                  display.drawRect(bx, 82, 6, 6);
        }

        display.drawText(5, HINT_Y, "Sel: cycle view   Back: exit");
    } while (display.nextPage());
}

// ─── Mode 2 – Full-screen QR ─────────────────────────────────────────────

void BadgeApp::renderQrFull(DisplayManager& display) {
    auto* stor = (_appManager && _appManager->context())
                 ? _appManager->context()->storage : nullptr;

    String qrData = stor ? stor->getBadgeQrData() : "";

    QRCode  qrcode;
    uint8_t buf[qrcode_getBufferSize(MAX_QR_VER)];
    bool    hasQr = false;

    if (!qrData.isEmpty()) {
        for (uint8_t v = 1; v <= MAX_QR_VER; v++) {
            if (qrcode_initText(&qrcode, buf, v, ECC_LOW, qrData.c_str())) {
                hasQr = true; break;
            }
        }
    }

    // Auto-scale: fit QR in the shorter display dimension with a 2px margin
    int scale = 1;
    if (hasQr && qrcode.size > 0) {
        scale = (display.height() - 4) / qrcode.size;
        if (scale < 1) scale = 1;
        if (scale > 8) scale = 8;
    }

    int side = hasQr ? (int)qrcode.size * scale : 0;
    int ox   = (display.width()  - side) / 2;
    int oy   = (display.height() - side) / 2;

    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.setTextBlack();

        if (hasQr) {
            for (uint8_t qy = 0; qy < qrcode.size; qy++) {
                for (uint8_t qx = 0; qx < qrcode.size; qx++) {
                    if (qrcode_getModule(&qrcode, qx, qy)) {
                        display.fillRect(ox + qx * scale, oy + qy * scale, scale, scale);
                    }
                }
            }
        } else {
            display.setDefaultFont();
            display.drawText(8, 55, "No QR data set.");
            display.drawText(8, 70, "Configure in portal.");
        }
    } while (display.nextPage());
}
