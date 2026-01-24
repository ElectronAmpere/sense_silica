#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
// Feature flags (compile-time)
// Defaults: enabled. Override via PlatformIO build_flags, e.g., -DENABLE_LCD=0 -DENABLE_SENSOR=0
#if !defined(ENABLE_LCD)
#define ENABLE_LCD 1
#endif
#if !defined(ENABLE_SENSOR)
#define ENABLE_SENSOR 1
#endif


// JSF AV C++ Rule 10: The #define directive shall not be used to create constants.
// Use const or constexpr instead.

// Pin configuration for Arduino Uno
namespace pins {
    // Serial communication with the computer
    constexpr long SERIAL_BAUD_RATE = 9600;

    // RS485 module connections
    constexpr uint8_t RE_PIN = 5;  // RS485-Module an DI
    constexpr uint8_t DE_PIN = 6;  // RS485-Module an RE und DE
    constexpr uint8_t RX_PIN = 2;  // RS485-Module an RO
    constexpr uint8_t TX_PIN = 3;  // RS485-Module an DI

    // LCD 16x2 connections
    constexpr uint8_t LCD_RS_PIN = 12;
    constexpr uint8_t LCD_EN_PIN = 11;
    constexpr uint8_t LCD_D4_PIN = 10;
    constexpr uint8_t LCD_D5_PIN = 9;
    constexpr uint8_t LCD_D6_PIN = 8;
    constexpr uint8_t LCD_D7_PIN = 7;

    // Built-in LED for status indication
    constexpr uint8_t LED_PIN_B5 = 13; // Standard Arduino Uno LED
}

// Task scheduling periods in milliseconds
namespace timing {
    constexpr uint32_t LED_TOGGLE_PERIOD_MS = 100;
    constexpr uint32_t SENSOR_READ_PERIOD_MS = 2000;
    constexpr uint32_t LCD_UPDATE_PERIOD_MS = 4000; // Slower update to reduce flicker
}

// UI configuration
namespace ui {
    // Total number of LCD pages to cycle through
    constexpr uint8_t LCD_PAGE_COUNT = 4; // (1) Temp/Moist, (2) pH/Cond, (3) NPK, (4) Status
}

namespace scheduler {

    /**
    * @brief The greatest common divisor (GCD) of all task periods, in milliseconds.
    * @details This value determines the fundamental tick rate of the scheduler.
    *          All task periods must be a multiple of this value.
    */
    constexpr uint32_t TASK_TICKS_GCD_IN_MS = 100; // GCD of 100, 2000, 2000 is 100

    /**
    * @brief The total number of non-idle tasks configured in the application.
    */
    constexpr uint8_t TOTAL_TASKS_NUM = 3; // LED, Sensor, and LCD tasks
    constexpr uint8_t TOTAL_TASKS_RUNNING_NUM = TOTAL_TASKS_NUM + 1;
    constexpr uint8_t IDLE_TASK_RUNNING_INDICATOR = 255;
}

#endif // CONFIG_H