#ifndef MODBUS_CLIENT_H
#define MODBUS_CLIENT_H

#include <Arduino.h>
#include <stdint.h>
#include "rs485.h"

typedef struct {
    Stream* io;                  // e.g., SoftwareSerial instance
    uint8_t rePin;               // RS485 Receiver Enable pin
    uint8_t dePin;               // RS485 Driver Enable pin
    bool reActiveLow;            // true for MAX485 (RE low = receiver enabled)
    bool deActiveHigh;           // true for MAX485 (DE high = driver enabled)
    rs485_config_t rs485;        // line settings (baud, data bits, parity, stop)
    uint16_t timeoutMs;          // per-frame timeout
    uint8_t maxRetries;          // number of request retries
    bool trace;                  // enable Serial trace logging
} ModbusClientConfig;

void modbus_client_init(ModbusClientConfig* cfg);

bool modbus_client_read_holding(ModbusClientConfig* cfg,
                                uint8_t address,
                                uint16_t regStart,
                                uint16_t qty,
                                uint16_t* outValues);

bool modbus_client_write_single(ModbusClientConfig* cfg,
                                uint8_t address,
                                uint16_t regAddr,
                                uint16_t value);

// Low-level diagnostic: send a raw request and read arbitrary response bytes
bool modbus_client_request_raw(ModbusClientConfig* cfg,
                               const uint8_t* req,
                               uint8_t reqLen,
                               uint8_t* outBuf,
                               uint8_t maxOut,
                               uint8_t* outLen);

// Probe helper: try a list of candidate device addresses on the current baud.
// Returns true and sets outAddr if a valid response is received from any.
bool modbus_client_probe_addresses(ModbusClientConfig* cfg,
                                   const uint8_t* addrs,
                                   uint8_t addrCount,
                                   uint8_t* outAddr);

#endif // MODBUS_CLIENT_H