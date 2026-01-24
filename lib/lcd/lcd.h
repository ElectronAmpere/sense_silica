#ifndef LCD_H
#define LCD_H

#include <Arduino.h>  // For pinMode, digitalWrite, delay (MISRA-compliant subset)

// LCD commands (HD44780/KS0066 compatible for JHD162A)
#define LCD_CLEAR_DISPLAY       0x01
#define LCD_RETURN_HOME         0x02
#define LCD_ENTRY_MODE_SET      0x04
#define LCD_DISPLAY_CONTROL     0x08
#define LCD_CURSOR_SHIFT        0x10
#define LCD_FUNCTION_SET        0x20
#define LCD_SET_CGRAM_ADDR      0x40
#define LCD_SET_DDRAM_ADDR      0x80

// Flags for display entry mode
#define LCD_ENTRY_RIGHT         0x00
#define LCD_ENTRY_LEFT          0x02
#define LCD_ENTRY_SHIFT_INC     0x01
#define LCD_ENTRY_SHIFT_DEC     0x00

// Flags for display on/off control
#define LCD_DISPLAY_ON          0x04
#define LCD_DISPLAY_OFF         0x00
#define LCD_CURSOR_ON           0x02
#define LCD_CURSOR_OFF          0x00
#define LCD_BLINK_ON            0x01
#define LCD_BLINK_OFF           0x00

// Flags for display/cursor shift
#define LCD_DISPLAY_MOVE        0x08
#define LCD_CURSOR_MOVE         0x00
#define LCD_MOVE_RIGHT          0x04
#define LCD_MOVE_LEFT           0x00

// Flags for function set
#define LCD_8BIT_MODE           0x10
#define LCD_4BIT_MODE           0x00
#define LCD_2_LINE              0x08
#define LCD_1_LINE              0x00
#define LCD_5x10_DOTS           0x04
#define LCD_5x8_DOTS            0x00

class LCD {
public:
    LCD(uint8_t rs, uint8_t en, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7, uint8_t backlight = 255);
    
    void begin();  // Initialize LCD
    void clear();  // Clear display
    void home();   // Return to home position
    void setCursor(uint8_t col, uint8_t row);  // Set cursor position (0-15 col, 0-1 row)
    void print(const char* str);  // Print string (bounded to 80 chars for JSF AV loop bound)
    void print(char c);           // Print single char
    void print(int num);          // Print integer (basic, no formatting)
    void print(float value, uint8_t decimals = 2U);  // Print float with specified decimal places (default 2)
    void displayOn();             // Turn display on
    void displayOff();            // Turn display off
    void cursorOn();              // Show cursor
    void cursorOff();             // Hide cursor
    void blinkOn();               // Blink cursor
    void blinkOff();              // Stop blinking
    void backlightOn();           // Turn backlight on
    void backlightOff();          // Turn backlight off
    void setBacklight(uint8_t brightness);  // PWM control (0-255)

private:
    uint8_t _rs_pin;
    uint8_t _en_pin;
    uint8_t _data_pins[4];  // D4-D7
    uint8_t _backlight_pin;
    uint8_t _display_function;
    uint8_t _display_control;
    uint8_t _display_mode;

    void send(uint8_t value, uint8_t mode);  // Send command or data
    void write4Bits(uint8_t value);          // Write 4-bit nibble
    void pulseEnable();                      // Pulse enable pin
};

#endif  // LCD_H