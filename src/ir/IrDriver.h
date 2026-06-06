#pragma once

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <vector>

struct IrCapture {
    bool valid = false;
    decode_type_t protocol = UNKNOWN;
    uint64_t value = 0;
    uint16_t bits = 0;
    std::vector<uint16_t> rawData;
};

class IrDriver {
public:
    IrDriver();

    bool begin();

    void startReceive();
    bool available();
    IrCapture readCapture();

    bool send(const IrCapture& capture);

    static String protocolToString(decode_type_t protocol);

private:
    static constexpr uint16_t IR_TX_PIN = 14;
    static constexpr uint16_t IR_RX_PIN = 21;

    static constexpr uint16_t IR_STATUS_LED_PIN = 2;

    // Buffer size and timeout are common knobs for receive stability.
    static constexpr uint16_t RECV_BUFFER_SIZE = 1024;
    static constexpr uint8_t RECV_TIMEOUT_MS = 50;
    static constexpr bool SAVE_BUFFER = true;

    IRrecv _irrecv;
    IRsend _irsend;
    decode_results _results;
    bool _started = false;
};