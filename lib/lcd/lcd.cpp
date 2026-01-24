#include "lcd.h"
#include <stdlib.h>  // For dtostrf() - already available in Arduino AVR environment

LCD::LCD(uint8_t rs, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, uint8_t backlight) {
    _rs_pin = rs;
    _en_pin = en;
    _data_pins[0] = d4;
    _data_pins[1] = d5;
    _data_pins[2] = d6;
    _data_pins[3] = d7;
    _backlight_pin = backlight;

    _display_function = LCD_4BIT_MODE | LCD_2_LINE | LCD_5x8_DOTS;
    _display_control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;
    _display_mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DEC;
}

void LCD::begin() {
    // Set pin modes (MISRA: explicit initialization)
    pinMode(_rs_pin, OUTPUT);
    pinMode(_en_pin, OUTPUT);
    for (uint8_t i = 0; i < 4U; ++i) {  // Bounded loop
        pinMode(_data_pins[i], OUTPUT);
    }
    if (_backlight_pin != 255U) {
        pinMode(_backlight_pin, OUTPUT);
        analogWrite(_backlight_pin, 255);  // Default full on
    } else {
        // No backlight control needed
    }

    // Initialization sequence (per KS0066/HD44780 datasheet for JHD162A)
    delay(50);  // Wait for power-up (>40ms)

    // Send function set in 8-bit mode initially
    write4Bits(0x03);
    delay(5);   // >4.1ms
    write4Bits(0x03);
    delay(1);   // >100us
    write4Bits(0x03);
    delay(1);
    write4Bits(0x02);  // Switch to 4-bit mode

    // Set function
    send(LCD_FUNCTION_SET | _display_function, 0);

    // Display on, cursor off
    send(LCD_DISPLAY_CONTROL | _display_control, 0);

    // Clear display
    clear();

    // Entry mode
    send(LCD_ENTRY_MODE_SET | _display_mode, 0);
}

void LCD::clear() {
    send(LCD_CLEAR_DISPLAY, 0);
    delay(2);  // >1.52ms
}

void LCD::home() {
    send(LCD_RETURN_HOME, 0);
    delay(2);  // >1.52ms
}

void LCD::setCursor(uint8_t col, uint8_t row) {
    if (row > 1U) {
        row = 1U;  // Clamp to 0-1 (MISRA: bounds check)
    } else {
        // No action needed for row <= 1U
    }
    if (col > 15U) {
        col = 15U;
    } else {
        // No action needed for col <= 15U
    }
    uint8_t addr = (row == 0U) ? col : (col + 0x40);
    send(LCD_SET_DDRAM_ADDR | addr, 0);
}

void LCD::print(const char* str) {
    const uint8_t max_len = 80U;  // Static upper bound for JSF AV loop compliance
    for (uint8_t i = 0U; i < max_len; ++i) {
        if (str[i] != '\0') {
            send(static_cast<uint8_t>(str[i]), 1);  // Data mode
        } else {
            break;  // Exit early if null terminator found
        }
    }
}

void LCD::print(char c) {
    send(static_cast<uint8_t>(c), 1);
}

void LCD::print(int num) {
    // Basic int to string (no sprintf for MISRA/AUTOSAR compliance)
    bool is_negative = false;
    if (num < 0) {
        print('-');
        num = -num;
        is_negative = true;
    } else {
        // No action for non-negative
    }

    if (num == 0 && !is_negative) {
        print('0');
    } else {
        char buf[6];  // Max for int16_t
        uint8_t i = 0U;
        while (num > 0 && i < 5U) {  // Bounded
            buf[i] = '0' + (num % 10);
            ++i;
            num /= 10;
        }
        while (i > 0U) {
            --i;
            print(buf[i]);
        }
    }
    // Single exit point
}

void LCD::print(float value, uint8_t decimals) {
    char buf[16];  // Sufficient size: sign + digits + '.' + decimals + null (~ e.g. -123.456 → 8 chars + margin)

    // Convert float to string: width=- (left align), precision=decimals
    dtostrf(value, -10, decimals, buf);  // -10 = left justify, enough width to avoid truncation

    // Trim leading/trailing spaces if any (dtostrf pads with spaces for positive width)
    char* p = buf;
    while (*p == ' ') ++p;  // Skip leading spaces

    print(p);  // Reuse existing print(const char*) - bounded and safe
}

void LCD::displayOn() {
    _display_control |= LCD_DISPLAY_ON;
    send(LCD_DISPLAY_CONTROL | _display_control, 0);
}

void LCD::displayOff() {
    _display_control &= ~LCD_DISPLAY_ON;
    send(LCD_DISPLAY_CONTROL | _display_control, 0);
}

void LCD::cursorOn() {
    _display_control |= LCD_CURSOR_ON;
    send(LCD_DISPLAY_CONTROL | _display_control, 0);
}

void LCD::cursorOff() {
    _display_control &= ~LCD_CURSOR_ON;
    send(LCD_DISPLAY_CONTROL | _display_control, 0);
}

void LCD::blinkOn() {
    _display_control |= LCD_BLINK_ON;
    send(LCD_DISPLAY_CONTROL | _display_control, 0);
}

void LCD::blinkOff() {
    _display_control &= ~LCD_BLINK_ON;
    send(LCD_DISPLAY_CONTROL | _display_control, 0);
}

void LCD::backlightOn() {
    if (_backlight_pin != 255U) {
        analogWrite(_backlight_pin, 255);
    } else {
        // No backlight control
    }
}

void LCD::backlightOff() {
    if (_backlight_pin != 255U) {
        analogWrite(_backlight_pin, 0);
    } else {
        // No backlight control
    }
}

void LCD::setBacklight(uint8_t brightness) {
    if (_backlight_pin != 255U) {
        analogWrite(_backlight_pin, brightness);
    } else {
        // No backlight control
    }
}

void LCD::send(uint8_t value, uint8_t mode) {
    digitalWrite(_rs_pin, mode);  // 0: command, 1: data
    write4Bits(value >> 4);       // High nibble
    write4Bits(value & 0x0F);     // Low nibble
}

void LCD::write4Bits(uint8_t value) {
    for (uint8_t i = 0U; i < 4U; ++i) {
        digitalWrite(_data_pins[i], (value >> i) & 0x01);
    }
    pulseEnable();
}

void LCD::pulseEnable() {
    digitalWrite(_en_pin, LOW);
    delayMicroseconds(1);
    digitalWrite(_en_pin, HIGH);
    delayMicroseconds(1);  // >450ns enable pulse (exceeds JHD162A min 230ns)
    digitalWrite(_en_pin, LOW);
    delayMicroseconds(100);  // >37us command exec (covers JHD162A timings)
}