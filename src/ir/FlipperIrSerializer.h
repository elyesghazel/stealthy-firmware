#pragma once
#include <Arduino.h>
#include <vector>
#include "ir/IrDriver.h"

// Converts IrCapture objects back to Flipper Zero .ir file format.
class FlipperIrSerializer {
public:
    // Serialize one signal. Returns empty string if capture cannot be serialized.
    static String serializeSignal(const String& name, const IrCapture& capture);

    // Full .ir file: header + all signals.
    static String serializeFile(const std::vector<String>& names,
                                const std::vector<IrCapture>& captures);

private:
    static String getFlipperProto(const IrCapture& cap);
    static bool   decodeValue(const IrCapture& cap, const String& flipProto,
                              uint8_t addr[4], uint8_t cmd[4]);
    static String hexBytesStr(const uint8_t* bytes, int len);
};
