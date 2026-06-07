#include "FlipperIrSerializer.h"
#include <IRutils.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

String FlipperIrSerializer::hexBytesStr(const uint8_t* bytes, int len) {
    String out;
    char buf[3];
    for (int i = 0; i < len; i++) {
        if (i > 0) out += ' ';
        snprintf(buf, sizeof(buf), "%02X", bytes[i]);
        out += buf;
    }
    return out;
}

String FlipperIrSerializer::getFlipperProto(const IrCapture& cap) {
    switch (cap.protocol) {
        case NEC:
            // Distinguish NEC (8-bit addr, inverted) from NECext (16-bit addr)
            {
                uint8_t b3 = (cap.value >> 24) & 0xFF;
                uint8_t b2 = (cap.value >> 16) & 0xFF;
                return ((b3 ^ b2) == 0xFF) ? "NEC" : "NECext";
            }
        case SAMSUNG: return "Samsung32";
        case SONY:
            if (cap.bits == 15) return "SIRC15";
            if (cap.bits == 20) return "SIRC20";
            return "SIRC";
        case RC5:     return "RC5";
        case RC6:     return "RC6";
        case JVC:     return "JVC";
        default:      return "";
    }
}

bool FlipperIrSerializer::decodeValue(const IrCapture& cap, const String& flipProto,
                                       uint8_t addr[4], uint8_t cmd[4]) {
    memset(addr, 0, 4);
    memset(cmd,  0, 4);
    uint64_t v = cap.value;

    if (flipProto == "NEC") {
        addr[0] = (uint8_t)reverseBits((v >> 24) & 0xFF, 8);
        cmd[0]  = (uint8_t)reverseBits((v >>  8) & 0xFF, 8);
    } else if (flipProto == "NECext") {
        // v = (revA16 << 16) | (revC << 8) | ~revC
        // revA16 high byte is bits 31-24, low byte is bits 23-16
        uint16_t revA16 = (uint16_t)(((v >> 24) & 0xFF) << 8) | ((v >> 16) & 0xFF);
        uint16_t a16    = (uint16_t)reverseBits(revA16, 16);
        addr[0] = a16 & 0xFF;
        addr[1] = (a16 >> 8) & 0xFF;
        cmd[0]  = (uint8_t)reverseBits((v >> 8) & 0xFF, 8);
    } else if (flipProto == "Samsung32") {
        addr[0] = (uint8_t)reverseBits((v >> 24) & 0xFF, 8);
        cmd[0]  = (uint8_t)reverseBits((v >>  8) & 0xFF, 8);
    } else if (flipProto == "SIRC") {
        uint32_t r = (uint32_t)reverseBits(v, 12);
        cmd[0]  = r & 0x7F;
        addr[0] = (r >> 7) & 0x1F;
    } else if (flipProto == "SIRC15") {
        uint32_t r = (uint32_t)reverseBits(v, 15);
        cmd[0]  = r & 0x7F;
        addr[0] = (r >> 7) & 0xFF;
    } else if (flipProto == "SIRC20") {
        uint32_t r   = (uint32_t)reverseBits(v, 20);
        cmd[0]       = r & 0x7F;
        uint16_t a13 = (r >> 7) & 0x1FFF;
        addr[0] = a13 & 0xFF;
        addr[1] = (a13 >> 8) & 0x1F;
    } else if (flipProto == "RC5") {
        addr[0] = (v >> 6) & 0x1F;
        cmd[0]  = v & 0x3F;
    } else if (flipProto == "RC6") {
        addr[0] = (v >> 8) & 0xFF;
        cmd[0]  = v & 0xFF;
    } else if (flipProto == "JVC") {
        uint32_t r = (uint32_t)reverseBits(v, 16);
        addr[0] = r & 0xFF;
        cmd[0]  = (r >> 8) & 0xFF;
    } else {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------

String FlipperIrSerializer::serializeSignal(const String& name, const IrCapture& cap) {
    if (!cap.valid) return "";

    String out;
    out += "name: ";
    out += name;
    out += '\n';

    if (!cap.rawData.empty()) {
        out += "type: raw\n";
        out += "frequency: 38000\n";
        out += "duty_cycle: 0.330000\n";
        out += "data:";
        for (size_t i = 0; i < cap.rawData.size(); i++) {
            out += ' ';
            out += String(cap.rawData[i]);
        }
        out += '\n';
        return out;
    }

    String flipProto = getFlipperProto(cap);
    if (flipProto.isEmpty()) return "";

    uint8_t addr[4], cmd[4];
    if (!decodeValue(cap, flipProto, addr, cmd)) return "";

    out += "type: parsed\n";
    out += "protocol: "; out += flipProto; out += '\n';
    out += "address: "; out += hexBytesStr(addr, 4); out += '\n';
    out += "command: "; out += hexBytesStr(cmd,  4); out += '\n';
    return out;
}

String FlipperIrSerializer::serializeFile(const std::vector<String>& names,
                                           const std::vector<IrCapture>& captures) {
    String out = "Filetype: IR signals file\nVersion: 1\n";
    size_t n = names.size() < captures.size() ? names.size() : captures.size();
    for (size_t i = 0; i < n; i++) {
        String block = serializeSignal(names[i], captures[i]);
        if (!block.isEmpty()) out += block;
    }
    return out;
}
