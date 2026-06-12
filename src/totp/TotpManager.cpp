#include "TotpManager.h"
#include <LittleFS.h>
#include <mbedtls/md.h>

// ─── Time ───────────────────────────────────────────────────────────────────

void TotpManager::syncTime(unsigned long unixSeconds) {
    _timeOffset    = unixSeconds - (unsigned long)(millis() / 1000UL);
    _timeSynced    = true;
    _timeFromFlash = false;
    saveTime();
    Serial.printf("[TOTP] Time synced: unix=%lu\n", unixSeconds);
}

void TotpManager::saveTime() const {
    if (!_timeSynced) return;
    File f = LittleFS.open(TIME_PATH, "w");
    if (!f) return;
    f.println(currentUnixTime());
    f.close();
}


unsigned long TotpManager::currentUnixTime() const {
    return _timeOffset + (unsigned long)(millis() / 1000UL);
}

int TotpManager::secondsElapsed() const {
    return (int)(currentUnixTime() % 30);
}

int TotpManager::secondsRemaining() const {
    return 29 - secondsElapsed();
}

// ─── Storage ────────────────────────────────────────────────────────────────

bool TotpManager::begin() {
    _accounts.clear();

    // Restore last known unix time from flash so TOTP works after a quick reboot
    // without needing a portal sync. Error = time the device was powered off.
    // Touch with "a" first so the "r" open below never hits a missing-file error.
    { File touch = LittleFS.open(TIME_PATH, "a"); if (touch) touch.close(); }
    {
        File tf = LittleFS.open(TIME_PATH, "r");
        if (tf) {
            unsigned long saved = (unsigned long)tf.readStringUntil('\n').toInt();
            tf.close();
            if (saved > 1000000000UL) {
                _timeOffset    = saved - (unsigned long)(millis() / 1000UL);
                _timeSynced    = true;
                _timeFromFlash = true;
                Serial.printf("[TOTP] Restored time from flash: ~%lu\n", saved);
            }
        }
    }

    if (!LittleFS.exists(PATH)) return true; // no accounts yet

    File f = LittleFS.open(PATH, "r");
    if (!f) return false;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) continue;
        int tab = line.indexOf('\t');
        if (tab < 1) continue;
        TotpAccount acct;
        acct.name   = line.substring(0, tab);
        acct.secret = line.substring(tab + 1);
        acct.secret.trim();
        if (_accounts.size() < (size_t)MAX_ACCOUNTS)
            _accounts.push_back(acct);
    }
    f.close();

    Serial.printf("[TOTP] Loaded %d account(s)\n", (int)_accounts.size());
    return true;
}

bool TotpManager::save() const {
    File f = LittleFS.open(PATH, "w");
    if (!f) return false;

    for (const auto& acct : _accounts) {
        f.print(acct.name);
        f.print('\t');
        f.println(acct.secret);
    }
    f.close();
    return true;
}

// ─── Account management ─────────────────────────────────────────────────────

bool TotpManager::addAccount(const String& name, const String& secret) {
    if ((int)_accounts.size() >= MAX_ACCOUNTS) return false;
    if (name.isEmpty() || secret.isEmpty())     return false;

    // Validate: try to decode the secret
    uint8_t buf[64];
    if (base32Decode(secret.c_str(), buf, sizeof(buf)) <= 0) return false;

    TotpAccount acct;
    acct.name   = name;
    acct.secret = secret;
    _accounts.push_back(acct);
    return save();
}

bool TotpManager::deleteAccount(int index) {
    if (index < 0 || index >= (int)_accounts.size()) return false;
    _accounts.erase(_accounts.begin() + index);
    return save();
}

// ─── TOTP code generation ───────────────────────────────────────────────────

String TotpManager::generateCode(int index) const {
    if (index < 0 || index >= (int)_accounts.size()) return "------";

    uint8_t key[64];
    int keyLen = base32Decode(_accounts[index].secret.c_str(), key, sizeof(key));
    if (keyLen <= 0) return "------";

    uint64_t counter = (uint64_t)currentUnixTime() / 30;
    uint32_t code    = computeTotp(key, (size_t)keyLen, counter);

    char buf[7];
    snprintf(buf, sizeof(buf), "%06lu", (unsigned long)code);
    return String(buf);
}

// ─── HMAC-SHA1 (RFC 4226 / RFC 6238) ───────────────────────────────────────

uint32_t TotpManager::computeTotp(const uint8_t* key, size_t keyLen, uint64_t counter) {
    // Big-endian 8-byte counter
    uint8_t msg[8];
    for (int i = 7; i >= 0; i--) {
        msg[i] = (uint8_t)(counter & 0xFF);
        counter >>= 8;
    }

    uint8_t hmac[20];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    mbedtls_md_setup(&ctx, info, 1 /* use HMAC */);
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, msg, 8);
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    // Dynamic truncation (RFC 4226 §5.4)
    int      offset = hmac[19] & 0x0F;
    uint32_t code   = ((uint32_t)(hmac[offset]     & 0x7F) << 24)
                    | ((uint32_t) hmac[offset + 1]          << 16)
                    | ((uint32_t) hmac[offset + 2]          <<  8)
                    |  (uint32_t) hmac[offset + 3];
    return code % 1000000U;
}

// ─── Base32 decode (RFC 4648) ────────────────────────────────────────────────

int TotpManager::base32Decode(const char* src, uint8_t* dst, size_t dstMax) {
    uint32_t bits  = 0;
    int      nbits = 0;
    size_t   out   = 0;

    for (; *src && out < dstMax; src++) {
        char c = (char)toupper((unsigned char)*src);
        if (c == '=' || c == ' ' || c == '\n' || c == '\r') continue;

        int val;
        if      (c >= 'A' && c <= 'Z') val = c - 'A';
        else if (c >= '2' && c <= '7') val = c - '2' + 26;
        else continue; // skip invalid chars

        bits   = (bits << 5) | (uint32_t)val;
        nbits += 5;
        if (nbits >= 8) {
            dst[out++] = (uint8_t)((bits >> (nbits - 8)) & 0xFF);
            nbits -= 8;
        }
    }
    return (int)out;
}
