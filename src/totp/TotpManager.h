#pragma once
#include <Arduino.h>
#include <vector>

struct TotpAccount {
    String name;
    String secret; // base32-encoded
};

class TotpManager {
public:
    bool begin();
    void syncTime(unsigned long unixSeconds);
    void tick();   // call from main loop — persists time to flash every 5 s

    bool          isTimeSynced()    const { return _timeSynced; }
    bool          isTimeFromFlash() const { return _timeFromFlash; }
    unsigned long currentUnixTime() const;
    int           secondsRemaining() const;
    int           secondsElapsed()   const;

    const std::vector<TotpAccount>& accounts() const { return _accounts; }
    bool   addAccount(const String& name, const String& secret);
    bool   deleteAccount(int index);
    String generateCode(int index) const;
    bool   save() const;

private:
    static uint32_t computeTotp(const uint8_t* key, size_t keyLen, uint64_t counter);
    static int      base32Decode(const char* src, uint8_t* dst, size_t dstMax);
    void            saveTime() const;

    std::vector<TotpAccount> _accounts;
    unsigned long _timeOffset    = 0;
    bool          _timeSynced    = false;
    bool          _timeFromFlash = false;
    unsigned long _lastTimeSaveMs = 0;

    static constexpr const char* PATH         = "/config/totp_accounts.txt";
    static constexpr const char* TIME_PATH    = "/config/totp_time.txt";
    static constexpr int         MAX_ACCOUNTS = 10;
    static constexpr unsigned long SAVE_INTERVAL_MS = 5000;
};
