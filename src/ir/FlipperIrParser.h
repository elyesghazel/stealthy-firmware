#pragma once
#include <Arduino.h>
#include <vector>
#include "ir/IrDriver.h"

struct FlipperSignal {
    String    name;
    IrCapture capture;
};

// Parses Flipper Zero .ir file format (both parsed and raw signal types).
class FlipperIrParser {
public:
    static std::vector<FlipperSignal> parse(const String& content);

private:
    static bool parseParsed(const String& proto, const String& addrStr,
                             const String& cmdStr, IrCapture& out);
    static bool parseRaw(const String& data, IrCapture& out);

    static int           hexBytes(const String& s, uint8_t* out, int maxLen);
    static decode_type_t mapProto(const String& name, uint16_t& bits);
};
