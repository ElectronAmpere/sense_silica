#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
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
            char line1[17];
            char line2[17];
            char tempBuf[12];
            char moistBuf[12];

            dtostrf(gSensorData.temperature, 0, 1, tempBuf);
            dtostrf(gSensorData.moisture, 0, 1, moistBuf);

            snprintf(line1, sizeof(line1), "Temp:%s degC", tempBuf);
            snprintf(line2, sizeof(line2), "Moist:%s %%", moistBuf);

            gLcd.setCursor(0U, 0U);
            gLcd.print(line1);
            gLcd.setCursor(0U, 1U);
            gLcd.print(line2);
            break;
        }
        case 1U: { // pH & Conductivity
            char line1[17];
            char line2[17];
            char phBuf[12];

            dtostrf(gSensorData.ph, 0, 2, phBuf);
            snprintf(line1, sizeof(line1), "pH:%s", phBuf);
            snprintf(line2, sizeof(line2), "Cond:%d uS", static_cast<int>(gSensorData.conductivity));

            gLcd.setCursor(0U, 0U);
            gLcd.print(line1);
            gLcd.setCursor(0U, 1U);
            gLcd.print(line2);
            break;
        }
        case 2U: { // N, P, K
            char line1[17];
            char line2[17];
            snprintf(line1, sizeof(line1), "N:%d P:%d",
                     static_cast<int>(gSensorData.nitrogen),
                     static_cast<int>(gSensorData.phosphorus));
            snprintf(line2, sizeof(line2), "K:%d mg/kg",
                     static_cast<int>(gSensorData.potassium));

            gLcd.setCursor(0U, 0U);
            gLcd.print(line1);
            gLcd.setCursor(0U, 1U);
            gLcd.print(line2);
            break;
        }
        default: { // Status page: Baud + last read status
            char line1[17];
            char line2[17];
            snprintf(line1, sizeof(line1), "Baud:%d", static_cast<int>(pins::SERIAL_BAUD_RATE));
            snprintf(line2, sizeof(line2), "Status:%s", gLastReadOk ? "OK" : "ERR");

            gLcd.setCursor(0U, 0U);
            gLcd.print(line1);
            gLcd.setCursor(0U, 1U);
            gLcd.print(line2);
            break;
        }
    }
    #else
    (void)page; (void)totalPages; // suppress unused warnings
    #endif

    page = static_cast<uint8_t>((page + 1U) % totalPages);
    return state;
}
