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

    bool          isTimeSynced()    const { return _timeSynced; }
    unsigned long currentUnixTime() const;
    int           secondsRemaining() const;  // 0–29 until next window
    int           secondsElapsed()   const;  // 0–29 elapsed in current window

    const std::vector<TotpAccount>& accounts() const { return _accounts; }
    bool   addAccount(const String& name, const String& secret);
    bool   deleteAccount(int index);
    String generateCode(int index) const;
    bool   save() const;

private:
    static uint32_t computeTotp(const uint8_t* key, size_t keyLen, uint64_t counter);
    static int      base32Decode(const char* src, uint8_t* dst, size_t dstMax);

    std::vector<TotpAccount> _accounts;
    unsigned long _timeOffset = 0; // unix_time - millis()/1000
    bool          _timeSynced = false;

    static constexpr const char* PATH         = "/config/totp_accounts.txt";
    static constexpr int         MAX_ACCOUNTS = 10;
};
