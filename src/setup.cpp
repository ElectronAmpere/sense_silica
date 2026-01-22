#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ModbusMaster.h"
#include "config.h"
#include "scheduler.h"
#include "timer.h"
#include "setup.h"
#include "tasks.h"

// Hardware instances
SoftwareSerial mySerial(pins::RX_PIN, pins::TX_PIN);
ModbusMaster node;
SoilSensor gSensor(node, pins::RE_PIN, pins::DE_PIN);
LCD gLcd(pins::LCD_RS_PIN, pins::LCD_EN_PIN, pins::LCD_D4_PIN, pins::LCD_D5_PIN, pins::LCD_D6_PIN, pins::LCD_D7_PIN);

void setupHardware() {
    pinMode(pins::LED_PIN_B5, OUTPUT);

    Serial.begin(pins::SERIAL_BAUD_RATE);
    mySerial.begin(pins::SERIAL_BAUD_RATE);
    #if ENABLE_SENSOR
    gSensor.begin(mySerial, pins::SERIAL_BAUD_RATE);
    #endif
    #if ENABLE_LCD
    gLcd.begin();
    #endif

    Serial.println("Soil Sensor Test - JSF Compliant Version");
}

void setupScheduler() {
    scheduler_init(tasks, scheduler::TOTAL_TASKS_NUM);
    timer1_set_period_ms(scheduler::TASK_TICKS_GCD_IN_MS);
}
