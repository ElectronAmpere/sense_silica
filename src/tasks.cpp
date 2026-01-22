#include <Arduino.h>
#include "config.h"
#include "scheduler.h"
#include "tasks.h"
#include "setup.h"
#include "lcd.h"

// Shared data
SoilSensor::SensorData gSensorData;
bool gLastReadOk = false;

// Tasks array
scheduler::Task tasks[scheduler::TOTAL_TASKS_NUM] = {
    scheduler::Task(timing::LED_TOGGLE_PERIOD_MS, &Task_ToggleLED),
    scheduler::Task(timing::SENSOR_READ_PERIOD_MS, &Task_SoilSensor),
    scheduler::Task(timing::LCD_UPDATE_PERIOD_MS, &Task_LcdUpdate)
};

int Task_ToggleLED(int state) {
    digitalWrite(pins::LED_PIN_B5, !digitalRead(pins::LED_PIN_B5));
    return state;
}

int Task_SoilSensor(int state) {
    #if ENABLE_SENSOR
    if (!gSensor.readAll(gSensorData)) {
        Serial.println("Failed to read from sensor!");
        gLastReadOk = false;
    } else {
        gLastReadOk = true;
    }
    #else
    (void)gSensorData; // suppress unused warning
    gLastReadOk = false;
    #endif
    return state;
}

int Task_LcdUpdate(int state) {
    static uint8_t page = 0U;
    const uint8_t totalPages = ui::LCD_PAGE_COUNT;

    #if ENABLE_LCD
    gLcd.clear();

    switch (page) {
        case 0U: { // Temperature & Moisture
            gLcd.setCursor(0U, 0U);
            gLcd.print("Temp:");
            gLcd.print(gSensorData.temperature, 1U);
            gLcd.print("C");

            gLcd.setCursor(0U, 1U);
            gLcd.print("Moist:");
            gLcd.print(gSensorData.moisture, 1U);
            gLcd.print("%");
            break;
        }
        case 1U: { // pH & Conductivity
            gLcd.setCursor(0U, 0U);
            gLcd.print("pH:");
            gLcd.print(gSensorData.ph, 2U);

            gLcd.setCursor(0U, 1U);
            gLcd.print("Cond:");
            gLcd.print(static_cast<int>(gSensorData.conductivity));
            gLcd.print("uS");
            break;
        }
        case 2U: { // N, P, K
            gLcd.setCursor(0U, 0U);
            gLcd.print("N:");
            gLcd.print(static_cast<int>(gSensorData.nitrogen));
            gLcd.print(" P:");
            gLcd.print(static_cast<int>(gSensorData.phosphorus));

            gLcd.setCursor(0U, 1U);
            gLcd.print("K:");
            gLcd.print(static_cast<int>(gSensorData.potassium));
            gLcd.print(" mg/kg");
            break;
        }
        default: { // Status page: Baud + last read status
            gLcd.setCursor(0U, 0U);
            gLcd.print("Baud:");
            gLcd.print(static_cast<int>(pins::SERIAL_BAUD_RATE));

            gLcd.setCursor(0U, 1U);
            gLcd.print("Status:");
            gLcd.print(gLastReadOk ? 'O' : 'E');
            gLcd.print(gLastReadOk ? "K" : "RR");
            break;
        }
    }
    #else
    (void)page; (void)totalPages; // suppress unused warnings
    #endif

    page = static_cast<uint8_t>((page + 1U) % totalPages);
    return state;
}
