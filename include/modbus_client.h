#ifndef MODBUS_CLIENT_H
#define MODBUS_CLIENT_H

#include <Arduino.h>
#include <stdint.h>
#include "rs485.h"

typedef struct {
    Stream* io;                  // e.g., SoftwareSerial instance
    uint8_t rePin;               // RS485 Receiver Enable (active high)
    uint8_t dePin;               // RS485 Driver Enable (active high)
    rs485_config_t rs485;        // line settings (baud, data bits, parity, stop)
    uint16_t timeoutMs;          // per-frame timeout
    uint8_t maxRetries;          // number of request retries
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

#endif // MODBUS_CLIENT_H