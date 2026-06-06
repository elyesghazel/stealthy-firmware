#include "FlipperIrParser.h"

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

std::vector<FlipperSignal> FlipperIrParser::parse(const String& content) {
    std::vector<FlipperSignal> out;

    String name, type, proto, addr, cmd, data;
    bool   inSignal = false;

    // Flush the current accumulator into `out`
    auto flush = [&]() {
        if (!inSignal || name.isEmpty()) return;
        FlipperSignal sig;
        sig.name = name;
        bool ok = false;
        if (type == "parsed")   ok = parseParsed(proto, addr, cmd, sig.capture);
        else if (type == "raw") ok = parseRaw(data, sig.capture);
        if (ok) out.push_back(sig);
        inSignal = false;
    };

    int pos = 0;
    int len = content.length();

    while (pos <= len) {
        int    nl   = content.indexOf('\n', pos);
        String line = (nl < 0) ? content.substring(pos) : content.substring(pos, nl);
        pos = (nl < 0) ? len + 1 : nl + 1;

        line.trim();
        if (line.isEmpty() || line[0] == '#') continue;

        int    colon = line.indexOf(':');
        if (colon < 0) continue;

        String key = line.substring(0, colon);
        String val = line.substring(colon + 1);
        key.trim(); val.trim();

        if (key == "name") {
            flush();
            name = val; type = ""; proto = ""; addr = ""; cmd = ""; data = "";
            inSignal = true;
        } else if (key == "type")     { type  = val; }
        else if (key == "protocol")   { proto = val; }
        else if (key == "address")    { addr  = val; }
        else if (key == "command")    { cmd   = val; }
        else if (key == "data")       { data  = val; }
        // Filetype, Version, frequency, duty_cycle → ignored
    }
    flush();
    return out;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int FlipperIrParser::hexBytes(const String& s, uint8_t* out, int maxLen) {
    auto hexN = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    int count = 0, i = 0, len = s.length();
    while (i < len && count < maxLen) {
        while (i < len && s[i] == ' ') i++;
        if (i + 1 >= len) break;
        int hi = hexN(s[i]), lo = hexN(s[i + 1]);
        if (hi < 0 || lo < 0) break;
        out[count++] = (uint8_t)((hi << 4) | lo);
        i += 2;
    }
    return count;
}

decode_type_t FlipperIrParser::mapProto(const String& name, uint16_t& bits) {
    if (name == "NEC")      { bits = 32; return NEC;     }
    if (name == "NECext")   { bits = 32; return NEC;     }
    if (name == "Samsung32"){ bits = 32; return SAMSUNG;  }
    if (name == "RC5")      { bits = 12; return RC5;      }
    if (name == "RC5X")     { bits = 13; return RC5;      }
    if (name == "RC6")      { bits = 20; return RC6;      }
    if (name == "SIRC")     { bits = 12; return SONY;     }
    if (name == "SIRC15")   { bits = 15; return SONY;     }
    if (name == "SIRC20")   { bits = 20; return SONY;     }
    if (name == "JVC")      { bits = 16; return JVC;      }
    if (name == "LG")       { bits = 28; return LG;       }
    bits = 32; return UNKNOWN;
}

bool FlipperIrParser::parseParsed(const String& proto, const String& addrStr,
                                   const String& cmdStr, IrCapture& out) {
    uint8_t a[4] = {}, c[4] = {};
    hexBytes(addrStr, a, 4);
    hexBytes(cmdStr,  c, 4);

    uint16_t bits = 32;
    decode_type_t dt = mapProto(proto, bits);
    if (dt == UNKNOWN) return false;

    uint64_t value = 0;

    if (proto == "NEC") {
        // 8-bit address + inverted + 8-bit command + inverted
        value = (uint64_t)a[0]
              | ((uint64_t)(~a[0] & 0xFF) << 8)
              | ((uint64_t)c[0] << 16)
              | ((uint64_t)(~c[0] & 0xFF) << 24);
    } else if (proto == "NECext") {
        // 16-bit address + 8-bit command + inverted (no address inversion)
        uint16_t a16 = a[0] | ((uint16_t)a[1] << 8);
        value = (uint64_t)a16
              | ((uint64_t)c[0] << 16)
              | ((uint64_t)(~c[0] & 0xFF) << 24);
    } else if (proto == "Samsung32") {
        // Each byte repeated: AABBCCDD where AA==BB, CC==DD
        value = (uint64_t)a[0] | ((uint64_t)a[0] << 8)
              | ((uint64_t)c[0] << 16) | ((uint64_t)c[0] << 24);
    } else if (proto == "SIRC") {
        value = (uint64_t)(c[0] & 0x7F) | ((uint64_t)(a[0] & 0x1F) << 7);
    } else if (proto == "SIRC15") {
        value = (uint64_t)(c[0] & 0x7F) | ((uint64_t)(a[0] & 0xFF) << 7);
    } else if (proto == "SIRC20") {
        uint16_t a13 = (a[0] | ((uint16_t)a[1] << 8)) & 0x1FFF;
        value = (uint64_t)(c[0] & 0x7F) | ((uint64_t)a13 << 7);
    } else if (proto == "RC5") {
        value = ((uint64_t)(a[0] & 0x1F) << 6) | (c[0] & 0x3F);
    } else if (proto == "RC5X") {
        value = ((uint64_t)(a[0] & 0x1F) << 7) | (c[0] & 0x7F);
    } else if (proto == "RC6") {
        value = ((uint64_t)a[0] << 8) | c[0];
    } else if (proto == "JVC") {
        value = (uint64_t)a[0] | ((uint64_t)c[0] << 8);
    } else if (proto == "LG") {
        // LG 28-bit: 8-bit address, 16-bit command, 4-bit nibble checksum
        uint16_t cmd16 = c[0] | ((uint16_t)c[1] << 8);
        uint8_t  chk   = (a[0] >> 4) + (a[0] & 0xF)
                       + (c[0] >> 4) + (c[0] & 0xF)
                       + (c[1] >> 4) + (c[1] & 0xF);
        value = ((uint64_t)a[0] << 20) | ((uint64_t)cmd16 << 4) | (chk & 0xF);
    } else {
        return false;
    }

    out.valid    = true;
    out.protocol = dt;
    out.value    = value;
    out.bits     = bits;
    return true;
}

bool FlipperIrParser::parseRaw(const String& data, IrCapture& out) {
    if (data.isEmpty()) return false;

    std::vector<uint16_t> timings;
    int pos = 0, len = data.length();

    while (pos < len) {
        while (pos < len && data[pos] == ' ') pos++;
        int end = pos;
        while (end < len && data[end] != ' ') end++;
        if (end > pos) {
            long v = data.substring(pos, end).toInt();
            if (v > 0) timings.push_back((uint16_t)min(v, 65535L));
        }
        pos = end;
    }

    if (timings.empty()) return false;

    out.valid    = true;
    out.protocol = UNKNOWN;
    out.value    = 0;
    out.bits     = 0;
    out.rawData  = timings;
    return true;
}
