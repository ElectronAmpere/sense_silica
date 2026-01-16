#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>
#include "rs485.h"

// Modbus-RTU frame layout:
// [Address (1B)] [Function (1B)] [Data (N B; high byte first for 16-bit)] [CRC (2B: low, high)]
typedef struct {
    uint8_t address;
    uint8_t function;
    const uint8_t* data;   // Pointer to data area (big-endian for 16-bit values)
    uint8_t data_len;      // Length of data area in bytes
    uint16_t crc;          // CRC16 over address+function+data (not including CRC)
} modbus_rtu_frame_t;

// Default function code used by the transmitter (read holding registers)
#define MODBUS_FUNC_READ_HOLDING_REGS (0x03)
#define MODBUS_FUNC_WRITE_SINGLE_REG   (0x06)

// Compute Modbus RTU CRC16 (polynomial 0xA001), over len bytes
static inline uint16_t modbus_rtu_crc16(const uint8_t* data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// Calculate required silent interval (≥ 4 character times) in microseconds
static inline unsigned long modbus_rtu_silent_interval_us(const rs485_config_t cfg)
{
    unsigned long char_bits = 1UL /* start */
                            + (unsigned long)cfg.dataBits
                            + (cfg.parity == RS485_PARITY_NONE ? 0UL : 1UL)
                            + (unsigned long)cfg.stopBits;
    return (char_bits * 4UL * 1000000UL) / cfg.baudRate;
}

// Build a read request (function 0x03) for qty registers starting at regAddr
// Output buffer layout: addr, func, regHi, regLo, qtyHi, qtyLo, crcLo, crcHi
static inline uint8_t modbus_rtu_build_read_request(uint8_t address,
                                                    uint16_t regAddr,
                                                    uint16_t qty,
                                                    uint8_t out[8])
{
    out[0] = address;
    out[1] = MODBUS_FUNC_READ_HOLDING_REGS;
    out[2] = (uint8_t)(regAddr >> 8);
    out[3] = (uint8_t)(regAddr & 0xFF);
    out[4] = (uint8_t)(qty >> 8);
    out[5] = (uint8_t)(qty & 0xFF);
    uint16_t crc = modbus_rtu_crc16(out, 6);
    out[6] = (uint8_t)(crc & 0xFF);
    out[7] = (uint8_t)((crc >> 8) & 0xFF);
    return 8;
}

// Build a write single register (function 0x06)
// Output buffer layout: addr, func, regHi, regLo, valHi, valLo, crcLo, crcHi
static inline uint8_t modbus_rtu_build_write_single(uint8_t address,
                                                    uint16_t regAddr,
                                                    uint16_t value,
                                                    uint8_t out[8])
{
    out[0] = address;
    out[1] = MODBUS_FUNC_WRITE_SINGLE_REG;
    out[2] = (uint8_t)(regAddr >> 8);
    out[3] = (uint8_t)(regAddr & 0xFF);
    out[4] = (uint8_t)(value >> 8);
    out[5] = (uint8_t)(value & 0xFF);
    uint16_t crc = modbus_rtu_crc16(out, 6);
    out[6] = (uint8_t)(crc & 0xFF);
    out[7] = (uint8_t)((crc >> 8) & 0xFF);
    return 8;
}

// Parse a 0x03 (Read Holding Registers) response. Buffer layout:
// [addr][func][byteCount][data...][crcLo][crcHi]
static inline bool modbus_rtu_parse_read_response(const uint8_t* buf,
                                                  uint8_t len,
                                                  uint8_t address,
                                                  uint8_t* outByteCount,
                                                  const uint8_t** outDataStart)
{
    if (len < 5) {
        return false;
    }
    uint16_t crcCalc = modbus_rtu_crc16(buf, (uint8_t)(len - 2));
    uint16_t crcRx = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    if (crcCalc != crcRx) {
        return false;
    }
    if (buf[0] != address || buf[1] != MODBUS_FUNC_READ_HOLDING_REGS) {
        return false;
    }
    uint8_t byteCount = buf[2];
    if (len != (uint8_t)(3 + byteCount + 2)) {
        return false;
    }
    *outByteCount = byteCount;
    *outDataStart = &buf[3];
    return true;
}

// Validate a write single register (0x06) echo response
// [addr][func][regHi][regLo][valHi][valLo][crcLo][crcHi]
static inline bool modbus_rtu_validate_write_single_echo(const uint8_t* buf,
                                                         uint8_t len,
                                                         uint8_t address,
                                                         uint16_t regAddr,
                                                         uint16_t value)
{
    if (len != 8) return false;
    uint16_t crcCalc = modbus_rtu_crc16(buf, 6);
    uint16_t crcRx = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
    if (crcCalc != crcRx) return false;
    if (buf[0] != address || buf[1] != MODBUS_FUNC_WRITE_SINGLE_REG) return false;
    uint16_t r = ((uint16_t)buf[2] << 8) | (uint16_t)buf[3];
    uint16_t v = ((uint16_t)buf[4] << 8) | (uint16_t)buf[5];
    return (r == regAddr) && (v == value);
}

#endif // MODBUS_RTU_H