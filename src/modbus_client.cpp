#include "modbus_client.h"
#include "modbus_rtu.h"

static bool modbus_tx_request(ModbusClientConfig* cfg, const uint8_t* req, uint8_t len)
{
    digitalWrite(cfg->rePin, HIGH);
    digitalWrite(cfg->dePin, HIGH);
    delayMicroseconds(modbus_rtu_silent_interval_us(cfg->rs485));
    size_t written = cfg->io->write(req, len);
    digitalWrite(cfg->dePin, LOW);
    digitalWrite(cfg->rePin, LOW);
    return written == len;
}

void modbus_client_init(ModbusClientConfig* cfg)
{
    pinMode(cfg->rePin, OUTPUT);
    pinMode(cfg->dePin, OUTPUT);
    digitalWrite(cfg->rePin, LOW);
    digitalWrite(cfg->dePin, LOW);
    if (cfg->timeoutMs == 0) cfg->timeoutMs = 200;
    if (cfg->maxRetries == 0) cfg->maxRetries = 2;
}

bool modbus_client_read_holding(ModbusClientConfig* cfg,
                                uint8_t address,
                                uint16_t regStart,
                                uint16_t qty,
                                uint16_t* outValues)
{
    uint8_t req[8];
    modbus_rtu_build_read_request(address, regStart, qty, req);

    for (uint8_t attempt = 0; attempt <= cfg->maxRetries; ++attempt) {
        if (!modbus_tx_request(cfg, req, sizeof(req))) {
            continue;
        }

        // Read header: addr, func, byteCount
        uint8_t header[3];
        uint8_t hidx = 0;
        uint32_t waitedUs = 0;
        const uint32_t timeoutUs = (uint32_t)cfg->timeoutMs * 1000UL;

        while (hidx < 3 && waitedUs < timeoutUs) {
            if (cfg->io->available()) {
                int c = cfg->io->read();
                if (c >= 0) header[hidx++] = (uint8_t)c;
            } else {
                delayMicroseconds(100);
                waitedUs += 100;
            }
        }
        if (hidx < 3) {
            continue; // retry
        }

        if (header[0] != address || header[1] != MODBUS_FUNC_READ_HOLDING_REGS) {
            continue; // retry
        }
        uint8_t byteCount = header[2];
        if (byteCount != (uint8_t)(qty * 2)) {
            continue; // retry
        }

        // Read tail: data + crc
        uint8_t tailLen = (uint8_t)(byteCount + 2);
        uint8_t frameLen = (uint8_t)(3 + tailLen);
        if (frameLen > 3 + 64) {
            continue; // oversize
        }
        uint8_t frame[3 + 64];
        frame[0] = header[0];
        frame[1] = header[1];
        frame[2] = header[2];
        uint8_t tidx = 0;
        waitedUs = 0;
        while (tidx < tailLen && waitedUs < timeoutUs) {
            if (cfg->io->available()) {
                int c = cfg->io->read();
                if (c >= 0) frame[3 + tidx++] = (uint8_t)c;
            } else {
                delayMicroseconds(100);
                waitedUs += 100;
            }
        }
        if (tidx < tailLen) {
            continue; // retry
        }

        uint8_t bc; const uint8_t* dataStart;
        if (!modbus_rtu_parse_read_response(frame, frameLen, address, &bc, &dataStart)) {
            continue; // retry
        }

        for (uint16_t i = 0; i < qty; ++i) {
            outValues[i] = ((uint16_t)dataStart[2*i] << 8) | (uint16_t)dataStart[2*i + 1];
        }
        return true;
    }

    return false;
}

bool modbus_client_write_single(ModbusClientConfig* cfg,
                                uint8_t address,
                                uint16_t regAddr,
                                uint16_t value)
{
    uint8_t req[8];
    modbus_rtu_build_write_single(address, regAddr, value, req);

    for (uint8_t attempt = 0; attempt <= cfg->maxRetries; ++attempt) {
        if (!modbus_tx_request(cfg, req, sizeof(req))) {
            continue;
        }
        // Expect fixed 8-byte echo
        uint8_t resp[8];
        uint8_t idx = 0;
        uint32_t waitedUs = 0;
        const uint32_t timeoutUs = (uint32_t)cfg->timeoutMs * 1000UL;
        while (idx < 8 && waitedUs < timeoutUs) {
            if (cfg->io->available()) {
                int c = cfg->io->read();
                if (c >= 0) resp[idx++] = (uint8_t)c;
            } else {
                delayMicroseconds(100);
                waitedUs += 100;
            }
        }
        if (idx < 8) continue;
        if (modbus_rtu_validate_write_single_echo(resp, 8, address, regAddr, value)) {
            return true;
        }
    }
    return false;
}