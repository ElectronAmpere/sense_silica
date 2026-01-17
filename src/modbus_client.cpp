#include "modbus_client.h"
#include "modbus_rtu.h"
#include "logger.h"

static void modbus_set_direction(ModbusClientConfig* cfg, bool tx)
{
    // Compute logical levels from polarity flags
    // TX: driver enabled, receiver disabled
    // RX: driver disabled, receiver enabled
    if (tx) {
        digitalWrite(cfg->dePin, cfg->deActiveHigh ? HIGH : LOW);
        digitalWrite(cfg->rePin, cfg->reActiveLow ? HIGH : LOW); // disable receiver
    } else {
        digitalWrite(cfg->dePin, cfg->deActiveHigh ? LOW : HIGH);
        digitalWrite(cfg->rePin, cfg->reActiveLow ? LOW : HIGH); // enable receiver
    }
}

static bool modbus_tx_request(ModbusClientConfig* cfg, const uint8_t* req, uint8_t len)
{
    modbus_set_direction(cfg, true);
    // Tutorial-aligned pre-TX idle (approx 10 ms)
    delay(10);

    size_t written = cfg->io->write(req, len);
    // Ensure transmission has fully completed based on baud and bits
    unsigned long char_bits = 1UL + (unsigned long)cfg->rs485.dataBits + (cfg->rs485.parity == RS485_PARITY_NONE ? 0UL : 1UL) + (unsigned long)cfg->rs485.stopBits;
    unsigned long char_time_us = (char_bits * 1000000UL) / cfg->rs485.baudRate;
    unsigned long tx_time_us = char_time_us * (unsigned long)len;
    delayMicroseconds(tx_time_us);
    // Guard before switching back to RX; extend to ~15 ms for diagnostics
    delay(15);
    modbus_set_direction(cfg, false);
    return written == len;
}

void modbus_client_init(ModbusClientConfig* cfg)
{
    pinMode(cfg->rePin, OUTPUT);
    pinMode(cfg->dePin, OUTPUT);
    // Default to RX state
    if (cfg->reActiveLow == false && cfg->deActiveHigh == false) {
        // If not initialized, set defaults for MAX485
        cfg->reActiveLow = true;
        cfg->deActiveHigh = true;
    }
    modbus_set_direction(cfg, false);
    if (cfg->timeoutMs == 0) cfg->timeoutMs = 500;
    if (cfg->maxRetries == 0) cfg->maxRetries = 2;
    if (cfg->trace) {
        LOGI("MODBUS", "client init");
        log_kv_u8("MODBUS", "rePin", "pin", cfg->rePin);
        log_kv_u8("MODBUS", "dePin", "pin", cfg->dePin);
        log_kv_u16("MODBUS", "baud", "bps", (uint16_t)cfg->rs485.baudRate);
        log_kv_u16("MODBUS", "timeout", "ms", cfg->timeoutMs);
        log_kv_u8("MODBUS", "retries", "n", cfg->maxRetries);
    }
}

bool modbus_client_read_holding(ModbusClientConfig* cfg,
                                uint8_t address,
                                uint16_t regStart,
                                uint16_t qty,
                                uint16_t* outValues)
{
    uint8_t req[8];
    modbus_rtu_build_read_request(address, regStart, qty, req);

    if (cfg->trace) {
        LOGD("MODBUS", "read holding");
        log_kv_hex8("MODBUS", "addr", "id", address);
        log_kv_hex16("MODBUS", "start", "reg", regStart);
        log_kv_u16("MODBUS", "qty", "regs", qty);
    }

    for (uint8_t attempt = 0; attempt <= cfg->maxRetries; ++attempt) {
        if (!modbus_tx_request(cfg, req, sizeof(req))) {
            if (cfg->trace) LOGW("MODBUS", "tx write mismatch");
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
            if (cfg->trace) LOGW("MODBUS", "timeout header");
            continue; // retry
        }

        if (header[0] != address || header[1] != MODBUS_FUNC_READ_HOLDING_REGS) {
            if (cfg->trace) LOGW("MODBUS", "addr/func mismatch");
            continue; // retry
        }
        uint8_t byteCount = header[2];
        if (byteCount != (uint8_t)(qty * 2)) {
            if (cfg->trace) LOGW("MODBUS", "byteCount mismatch");
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
            if (cfg->trace) LOGW("MODBUS", "timeout data");
            continue; // retry
        }

        uint8_t bc; const uint8_t* dataStart;
        if (!modbus_rtu_parse_read_response(frame, frameLen, address, &bc, &dataStart)) {
            if (cfg->trace) LOGW("MODBUS", "response parse failed");
            continue; // retry
        }

        for (uint16_t i = 0; i < qty; ++i) {
            outValues[i] = ((uint16_t)dataStart[2*i] << 8) | (uint16_t)dataStart[2*i + 1];
        }
        if (cfg->trace) {
            LOGI("MODBUS", "read ok");
            for (uint16_t i = 0; i < qty; ++i) {
                log_kv_hex16("MODBUS", "val", "reg", outValues[i]);
            }
        }
        return true;
    }

    if (cfg->trace) LOGE("MODBUS", "read failed");
    return false;
}

bool modbus_client_write_single(ModbusClientConfig* cfg,
                                uint8_t address,
                                uint16_t regAddr,
                                uint16_t value)
{
    uint8_t req[8];
    modbus_rtu_build_write_single(address, regAddr, value, req);

    if (cfg->trace) {
        LOGD("MODBUS", "write single");
        log_kv_hex8("MODBUS", "addr", "id", address);
        log_kv_hex16("MODBUS", "reg", "addr", regAddr);
        log_kv_hex16("MODBUS", "val", "value", value);
    }

    for (uint8_t attempt = 0; attempt <= cfg->maxRetries; ++attempt) {
        if (!modbus_tx_request(cfg, req, sizeof(req))) {
            if (cfg->trace) LOGW("MODBUS", "tx write mismatch");
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
        if (idx < 8) { if (cfg->trace) LOGW("MODBUS", "timeout write echo"); continue; }
        if (modbus_rtu_validate_write_single_echo(resp, 8, address, regAddr, value)) {
            if (cfg->trace) LOGI("MODBUS", "write ok");
            return true;
        }
        if (cfg->trace) LOGW("MODBUS", "write echo invalid");
    }
    if (cfg->trace) LOGE("MODBUS", "write failed");
    return false;
}

bool modbus_client_request_raw(ModbusClientConfig* cfg,
                               const uint8_t* req,
                               uint8_t reqLen,
                               uint8_t* outBuf,
                               uint8_t maxOut,
                               uint8_t* outLen)
{
    if (outLen) *outLen = 0;
    if (!modbus_tx_request(cfg, req, reqLen)) {
        if (cfg->trace) LOGE("MODBUS", "raw tx failed");
        return false;
    }

    uint32_t waitedUs = 0;
    const uint32_t timeoutUs = (uint32_t)cfg->timeoutMs * 1000UL;
    uint8_t idx = 0;
    while (waitedUs < timeoutUs && idx < maxOut) {
        if (cfg->io->available()) {
            int c = cfg->io->read();
            if (c >= 0) {
                outBuf[idx++] = (uint8_t)c;
            }
        } else {
            delayMicroseconds(100);
            waitedUs += 100;
        }
    }
    if (outLen) *outLen = idx;
    if (cfg->trace) {
        LOGI("MODBUS", "raw rx");
        for (uint8_t i = 0; i < idx; ++i) {
            log_kv_hex8("MODBUS", "byte", "b", outBuf[i]);
        }
    }
    return idx > 0;
}

bool modbus_client_probe_addresses(ModbusClientConfig* cfg,
                                   const uint8_t* addrs,
                                   uint8_t addrCount,
                                   uint8_t* outAddr)
{
    if (outAddr) *outAddr = 0;
    // Use a simple read holding register known to exist on these sensors (moisture 0x0012, qty=1)
    uint8_t req[8];
    for (uint8_t i = 0; i < addrCount; ++i) {
        uint8_t addr = addrs[i];
        if (cfg->trace) {
            LOGD("MODBUS", "probe try");
            log_kv_hex8("MODBUS", "addr", "id", addr);
        }
        modbus_rtu_build_read_request(addr, 0x0012, 1, req);
        if (!modbus_tx_request(cfg, req, sizeof(req))) {
            continue;
        }
        // Try to read a valid header and payload
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
            if (cfg->trace) LOGW("MODBUS", "probe timeout header");
            continue; // no header
        }
        if (header[0] != addr || header[1] != MODBUS_FUNC_READ_HOLDING_REGS) {
            if (cfg->trace) LOGW("MODBUS", "probe addr/func mismatch");
            continue; // mismatch
        }
        uint8_t byteCount = header[2];
        // For qty=1, expect 2 data bytes
        if (byteCount != 2) {
            if (cfg->trace) LOGW("MODBUS", "probe byteCount mismatch");
            continue;
        }
        // Read data + crc
        uint8_t tailLen = 2 + 2;
        uint8_t frameLen = 3 + tailLen;
        uint8_t frame[3 + 4];
        frame[0] = header[0]; frame[1] = header[1]; frame[2] = header[2];
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
            if (cfg->trace) LOGW("MODBUS", "probe timeout data");
            continue;
        }
        uint8_t bc; const uint8_t* dataStart;
        if (!modbus_rtu_parse_read_response(frame, frameLen, addr, &bc, &dataStart)) {
            if (cfg->trace) LOGW("MODBUS", "probe parse failed");
            continue;
        }
        if (outAddr) *outAddr = addr;
        if (cfg->trace) {
            LOGI("MODBUS", "probe hit");
            log_kv_hex8("MODBUS", "addr", "id", addr);
        }
        return true;
    }
    if (cfg->trace) LOGW("MODBUS", "probe no match");
    return false;
}