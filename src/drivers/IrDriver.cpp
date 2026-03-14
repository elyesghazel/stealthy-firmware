#include "IrDriver.h"

IrDriver::IrDriver()
    : _irrecv(IR_RX_PIN, RECV_BUFFER_SIZE, RECV_TIMEOUT_MS, SAVE_BUFFER),
      _irsend(IR_TX_PIN) {
}

bool IrDriver::begin() {
    _irsend.begin();
    _irrecv.enableIRIn();
    pinMode(IR_STATUS_LED_PIN, OUTPUT);
    digitalWrite(IR_STATUS_LED_PIN, LOW);
    _started = true;

    

    Serial.println("[IR] driver initialized");
    Serial.printf("[IR] TX pin: %u\n", IR_TX_PIN);
    Serial.printf("[IR] RX pin: %u\n", IR_RX_PIN);

    return true;
}

void IrDriver::startReceive() {
    if (!_started) return;
    _irrecv.resume();
}

bool IrDriver::available() {
    if (!_started) return false;
    return _irrecv.decode(&_results);
}

IrCapture IrDriver::readCapture() {
    IrCapture capture;

    if (!_started) {
        return capture;
    }

    if (!_results.decode_type) {
        // still allow UNKNOWN or other states through below
    }

    capture.valid = true;
    capture.protocol = _results.decode_type;
    capture.bits = _results.bits;
    capture.value = _results.value;

    capture.rawData.reserve(_results.rawlen);
    for (uint16_t i = 0; i < _results.rawlen; i++) {
        capture.rawData.push_back(_results.rawbuf[i] * kRawTick);
    }

    Serial.println("[IR] capture received");
    Serial.printf("[IR] protocol: %s\n", typeToString(_results.decode_type).c_str());
    Serial.printf("[IR] value   : 0x%llX\n", _results.value);
    Serial.printf("[IR] bits    : %u\n", _results.bits);
    Serial.printf("[IR] rawlen  : %u\n", _results.rawlen);

    _irrecv.resume();
    return capture;
}

bool IrDriver::send(const IrCapture& capture) {
    if (!_started || !capture.valid) {
        return false;
    }

    Serial.println("[IR] sending capture");
    digitalWrite(IR_STATUS_LED_PIN, HIGH);
    bool success = false;


    switch (capture.protocol) {
        case NEC:
            _irsend.sendNEC(capture.value, capture.bits);
            success = true;
            break;

        case SONY:
            _irsend.sendSony(capture.value, capture.bits);
            success = true;
            break;

        case RC5:
            _irsend.sendRC5(capture.value, capture.bits);
            success = true;
            break;

        case RC6:
            _irsend.sendRC6(capture.value, capture.bits);
            success = true;
            break;

        case JVC:
            _irsend.sendJVC(capture.value, capture.bits, false);
            success = true;
            break;

        case SAMSUNG:
            _irsend.sendSAMSUNG(capture.value, capture.bits);
            success = true;
            break;

        case LG:
            _irsend.sendLG(capture.value, capture.bits);
            success = true;
            break;

        default:
            break;
    }

    if (!success && !capture.rawData.empty()) {
        _irsend.sendRaw(
            capture.rawData.data(),
            static_cast<uint16_t>(capture.rawData.size()),
            38
        );
        success = true;
    }

    digitalWrite(IR_STATUS_LED_PIN, LOW);

    return success;
}

String IrDriver::protocolToString(decode_type_t protocol) {
    return typeToString(protocol).c_str();
}