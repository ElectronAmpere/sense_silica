#ifndef RS485_H
#define RS485_H

typedef enum {
    RS485_CONTENT_BINARY_8 = 0
} rs485_content_t;

typedef enum {
    RS485_PARITY_NONE = 0,
    RS485_PARITY_EVEN = 1,
    RS485_PARITY_ODD = 2
} rs485_parity_t;

typedef enum {
    RS485_ERROR_CHECK_CRC = 0
} rs485_error_check_t;

typedef struct {
    unsigned long baudRate;
    unsigned char dataBits;
    rs485_parity_t parity;
    unsigned char stopBits;
    rs485_error_check_t errorCheck;
    rs485_content_t contentCoding;
} rs485_config_t;

static const rs485_config_t NPK_RS485_DEFAULT = {
    9600UL,
    8,
    RS485_PARITY_NONE,
    1,
    RS485_ERROR_CHECK_CRC,
    RS485_CONTENT_BINARY_8
};

#endif // RS485_H