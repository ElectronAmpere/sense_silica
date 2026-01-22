#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ModbusMaster.h"
#include "SoilSensor.h"
#include "config.h"
#include "scheduler.h"
#include "timer.h"
#include "lcd.h"

// Forward declarations for task functions
int Task_ToggleLED(int state);
int Task_SoilSensor(int state);
int Task_LcdUpdate(int state);

// Static initializations
SoftwareSerial mySerial(pins::RX_PIN, pins::TX_PIN);
ModbusMaster node;
SoilSensor gSensor(node, pins::RE_PIN, pins::DE_PIN);
LCD gLcd(pins::LCD_RS_PIN, pins::LCD_EN_PIN, pins::LCD_D4_PIN, pins::LCD_D5_PIN, pins::LCD_D6_PIN, pins::LCD_D7_PIN);
SoilSensor::SensorData gSensorData; // Global sensor data, shared between tasks

// JSF AV C++ Rule 18: All variables shall be initialized.
avr_embedded::Task tasks[avr_embedded::TOTAL_TASKS_NUM] = {
    avr_embedded::Task(timing::LED_TOGGLE_PERIOD_MS, &Task_ToggleLED),
    avr_embedded::Task(timing::SENSOR_READ_PERIOD_MS, &Task_SoilSensor),
    avr_embedded::Task(timing::LCD_UPDATE_PERIOD_MS, &Task_LcdUpdate)
};

int Task_ToggleLED(int state) {
    // JSF AV C++ Rule 144: Direct use of bitwise operators is restricted.
    digitalWrite(pins::LED_PIN_B5, !digitalRead(pins::LED_PIN_B5));
    return state; // State is not used in this task
}

int Task_SoilSensor(int state) {
    // This task reads sensor data and stores it in the global gSensorData.
    if (!gSensor.readAll(gSensorData)) {
        // Error handling can be improved, e.g., setting a status flag
        Serial.println("Failed to read from sensor!");
    }
    return state; // State is not used in this task
}

int Task_LcdUpdate(int state) {
    // This task reads from the global gSensorData and displays it.
    gLcd.clear();
    gLcd.setCursor(0, 0);
    gLcd.print("T:");
    gLcd.print(gSensorData.temperature);
    gLcd.print(" M:");
    gLcd.print(gSensorData.moisture);
    gLcd.setCursor(0, 1);
    gLcd.print("pH:");
    gLcd.print(gSensorData.ph);
    return state;
}

// Forward declarations
void setupHardware();
void setupScheduler();

int main(void) {
    // Manually call the Arduino core init function.
    init();

    setupHardware();
    setupScheduler();
    
    // Enable global interrupts
    sei();

    // The scheduler takes over from here.
    while (true) {
        // This loop will be preempted by the timer interrupt for task scheduling.
        // It can be used for low-priority background processing or power-saving modes.
    }

    return 0; // This line is unreachable.
}

void setupHardware() {
    pinMode(pins::LED_PIN_B5, OUTPUT);

    Serial.begin(pins::SERIAL_BAUD_RATE);
    mySerial.begin(pins::SERIAL_BAUD_RATE);
    gSensor.begin(mySerial, pins::SERIAL_BAUD_RATE);
    gLcd.begin();

    Serial.println("Soil Sensor Test - JSF Compliant Version");
}

void setupScheduler() {
    scheduler_init(tasks, avr_embedded::TOTAL_TASKS_NUM);
    timer1_set_period_ms(avr_embedded::TASK_TICKS_GCD_IN_MS);
}