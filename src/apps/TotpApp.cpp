#include "TotpApp.h"
#include "framework/AppManager.h"
#include "framework/AppContext.h"
#include "display/DisplayManager.h"
#include "totp/TotpManager.h"

// ─── Display geometry (250×122, landscape, status bar y=0-19) ─────────────
namespace {
    // Account name
    constexpr int NAME_Y   = 29;   // default-font baseline, top at 21

    // Horizontal rule below name
    constexpr int RULE1_Y  = 33;

    // ── 6 digit boxes (FreeMonoBold18pt7b) ──────────────────────────────────
    // Font: xAdvance=21, cap height≈22, yOffset≈-22
    // Box: 25 wide × 28 tall, cursor placed 2px inside box left
    // Layout: 3 boxes · 2px gap · 10px group gap · 3 boxes · 2px gap
    // Total span: 3×25+2×2+10+3×25+2×2 = 168px  →  start x = (250-168)/2 = 41
    constexpr int BOX_W    = 25;
    constexpr int BOX_H    = 28;
    constexpr int DIG_PAD  = 2;    // left offset from box_x to cursor_x
    constexpr int DIG_GAP  = 2;    // gap between boxes in same group
    constexpr int GRP_GAP  = 10;   // gap between the two groups
    constexpr int CODE_BL  = 67;   // 18pt baseline y
    constexpr int CODE_X0  = 41;   // first box left x
    constexpr int CODE_SPAN = 168; // total span of all 6 boxes (used for bar)

    // box_x for each of the 6 digits
    constexpr int boxX(int d) {
        // d < 3: group 0; d >= 3: group 1
        return (d < 3)
            ? CODE_X0 + d * (BOX_W + DIG_GAP)
            : CODE_X0 + 3 * (BOX_W + DIG_GAP) + GRP_GAP + (d - 3) * (BOX_W + DIG_GAP);
    }

    // Horizontal rule below digit boxes
    constexpr int RULE2_Y  = 77;

    // Progress bar (same x/width as code boxes)
    constexpr int BAR_X    = CODE_X0;
    constexpr int BAR_Y    = 82;
    constexpr int BAR_H    = 7;
    constexpr int BAR_W    = CODE_SPAN;

    // Seconds text
    constexpr int SEC_Y    = 96;   // default-font baseline

    // Navigation
    constexpr int NAV_Y    = 114;

    // Partial update region (everything below the status bar)
    constexpr int PART_Y   = 20;
    constexpr int PART_H   = 102; // 122 - 20
}

// ─────────────────────────────────────────────────────────────────────────────

TotpApp::TotpApp() {}

void TotpApp::setup(AppManager* appManager, IApp* returnApp) {
    _appManager = appManager;
    _returnApp  = returnApp;
}

void TotpApp::onEnter() {
    Serial.println("[TotpApp] enter");
    auto* ctx = _appManager ? _appManager->context() : nullptr;
    _totp = ctx ? ctx->totp : nullptr;

    _selectedIndex  = 0;
    _accountCount   = _totp ? (int)_totp->accounts().size() : 0;
    _lastUpdateMs   = 0;
    _partialCount   = 0;
    requestFullRender();
}

void TotpApp::onExit() {
    Serial.println("[TotpApp] exit");
}

void TotpApp::handleButton(const ButtonEvent& event) {
    if (event.action != ButtonAction::Press) return;

    if (event.id == ButtonId::Back) {
        if (_appManager && _returnApp) _appManager->switchTo(_returnApp);
        return;
    }

    if (!_totp || _accountCount == 0) return;

    if (event.id == ButtonId::Up) {
        _selectedIndex = (_selectedIndex > 0) ? _selectedIndex - 1 : _accountCount - 1;
        requestFullRender();
    } else if (event.id == ButtonId::Down) {
        _selectedIndex = (_selectedIndex < _accountCount - 1) ? _selectedIndex + 1 : 0;
        requestFullRender();
    }
}

void TotpApp::update() {
    if (!_totp || !_totp->isTimeSynced() || _accountCount == 0) return;

    unsigned long now = millis();
    if (now - _lastUpdateMs < 2000) return;
    _lastUpdateMs = now;

    int secsLeft = _totp->secondsRemaining();

    // Full refresh every 30s when the code rolls over, or every 20 partials
    if (secsLeft != _lastSecondsRemaining) {
        bool codeChange = (secsLeft > _lastSecondsRemaining && _lastSecondsRemaining >= 0)
                        || (_lastSecondsRemaining < 0);
        _lastSecondsRemaining = secsLeft;

        if (codeChange || ++_partialCount >= 20) {
            _partialCount = 0;
            requestFullRender();
        } else {
            requestPartialRender();
        }
    }
}

// ─── Render ──────────────────────────────────────────────────────────────────

void TotpApp::render(DisplayManager& display) {
    if (!_needsRender) return;
    if (_renderMode == TotpRenderMode::Full) renderFull(display);
    else                                     renderPartial(display);
    _needsRender = false;
}

void TotpApp::requestFullRender()    { _renderMode = TotpRenderMode::Full;    _needsRender = true; }
void TotpApp::requestPartialRender() { _renderMode = TotpRenderMode::Partial; _needsRender = true; }

void TotpApp::renderFull(DisplayManager& display) {
    display.startFullWindowDraw();
    do {
        display.fillWhite();
        display.drawStatusBar();
        drawContent(display);
    } while (display.nextPage());
}

void TotpApp::renderPartial(DisplayManager& display) {
    int x = 0;
    int y = PART_Y;
    int w = ((display.width() + 7) / 8) * 8;
    int h = (PART_H % 2 == 0) ? PART_H : PART_H + 1;

    display.startPartialWindowDraw(x, y, w, h);
    do {
        display.fillRectWhite(x, y, w, h);
        drawContent(display);
    } while (display.nextPage());
}

void TotpApp::drawContent(DisplayManager& display) {
    display.setDefaultFont();
    display.setTextBlack();

    // No TOTP manager
    if (!_totp) {
        display.drawText(10, 60, "TOTP unavailable");
        return;
    }

    // No accounts configured
    if (_accountCount == 0) {
        drawNoAccounts(display);
        return;
    }

    // Time not synced
    if (!_totp->isTimeSynced()) {
        drawNoTimeSync(display);
        return;
    }

    // ── "~" indicator when time is from flash (approximate) ─────────────────
    if (_totp->isTimeFromFlash()) {
        display.drawText(display.width() - 10, NAME_Y, "~");
    }

    // ── Account name (centered) ──────────────────────────────────────────────
    const String& name = _totp->accounts()[_selectedIndex].name;
    // Center: each default-font char ≈ 6px wide
    int nameW = (int)name.length() * 6;
    int nameX = (display.width() - nameW) / 2;
    if (nameX < 4) nameX = 4;
    display.drawText(nameX, NAME_Y, name.c_str());

    // Rule under name
    display.fillRect(0, RULE1_Y, display.width(), 1);

    // ── Digit boxes ──────────────────────────────────────────────────────────
    String code = _totp->generateCode(_selectedIndex);
    drawDigitBoxes(display, code);

    // Rule under boxes
    display.fillRect(0, RULE2_Y, display.width(), 1);

    // ── Progress bar ─────────────────────────────────────────────────────────
    drawProgressBar(display);

    // ── Seconds remaining ────────────────────────────────────────────────────
    int secsLeft = _totp->secondsRemaining();
    char secBuf[12];
    snprintf(secBuf, sizeof(secBuf), "%ds", secsLeft + 1);
    // right-align inside bar area
    int secW = (int)strlen(secBuf) * 6;
    display.drawText(BAR_X + BAR_W - secW, SEC_Y, secBuf);

    // ── Navigation (N / M) ───────────────────────────────────────────────────
    if (_accountCount > 1) {
        char navBuf[20];
        snprintf(navBuf, sizeof(navBuf), "< %d / %d >", _selectedIndex + 1, _accountCount);
        int navW = (int)strlen(navBuf) * 6;
        int navX = (display.width() - navW) / 2;
        display.drawText(navX, NAV_Y, navBuf);
    } else {
        display.drawText((display.width() - 66) / 2, NAV_Y, "Back = exit");
    }
}

void TotpApp::drawDigitBoxes(DisplayManager& display, const String& code) {
    display.setCodeFont();
    display.setTextBlack();

    // Draw group separator dot in the gap between digits 2 and 3
    int sepX = boxX(2) + BOX_W + DIG_GAP + GRP_GAP / 2 - 1; // center of gap
    int sepY = CODE_BL - BOX_H / 2;
    display.fillRect(sepX, sepY, 2, 2); // 2×2 dot

    for (int d = 0; d < 6; d++) {
        int bx = boxX(d);
        int by = CODE_BL - BOX_H + 2; // 2px above top of glyph

        // Box border (1px)
        display.drawRect(bx, by, BOX_W, BOX_H);

        // Digit character
        char ch[2] = { (d < (int)code.length()) ? code[d] : '0', '\0' };
        display.drawText(bx + DIG_PAD, CODE_BL, ch);
    }

    display.setDefaultFont();
    display.setTextBlack();
}

void TotpApp::drawProgressBar(DisplayManager& display) {
    int secsLeft = _totp ? _totp->secondsRemaining() : 0;

    // Outer border
    display.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H);

    // Inner fill: proportional to remaining time (bar shrinks as time passes)
    int fillW = (int)((long)BAR_W * (secsLeft + 1) / 30);
    if (fillW > BAR_W - 2) fillW = BAR_W - 2;
    if (fillW > 0) {
        display.fillRect(BAR_X + 1, BAR_Y + 1, fillW, BAR_H - 2);
    }
}

void TotpApp::drawNoAccounts(DisplayManager& display) {
    display.setDefaultFont();
    display.drawText(10, 52, "No accounts yet.");
    display.drawText(10, 64, "Add them in the");
    display.drawText(10, 76, "portal TOTP tab.");
    display.drawText(10, 96, "Back = exit");
}

void TotpApp::drawNoTimeSync(DisplayManager& display) {
    display.setDefaultFont();
    display.drawText(10, 52, "Time not synced.");
    display.drawText(10, 64, "Open the TOTP tab");
    display.drawText(10, 76, "in the portal.");
    display.drawText(10, 96, "Back = exit");
}
