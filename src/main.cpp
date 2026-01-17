#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include "SoilSensor.h"

// Task type and scheduler API
#include "config.h"
#include "scheduler.h"
#include "timer.h"

#define RE_PIN 8
#define DE_PIN 7

int Task_ToggleLED(int state);
int Task_SoilSensor(int state);

void TimerSet(void)
{
    // Auto-reload via CTC at TASK_TICKS_GCD_IN_MS
    timer1_set_period_ms(TASK_TICKS_GCD_IN_MS);
    sei(); // Enable interrupt
}

static task tasks[TOTAL_TASKS_NUM] = {
    {0, 0, 100, 0, &Task_ToggleLED},   // Toggle LED every 100 ms
    {0, 0, 2000, 0, &Task_SoilSensor} // Read soil sensor every 2000 ms
};

SoftwareSerial mySerial(2, 3); // RX, TX
ModbusMaster node;
SoilSensor gSensor(node, RE_PIN, DE_PIN);

int Task_ToggleLED(int state) {
    PORTB ^= _BV(PORTB5);
    return state;
}

int Task_SoilSensor(int state) {
    SoilSensor::SensorData data;
    if (gSensor.readAll(data)) {
        Serial.print("Moisture: "); Serial.print(data.moisture); Serial.print("%\t");
        Serial.print("Temperature: "); Serial.print(data.temperature); Serial.print("C\t");
        Serial.print("Conductivity: "); Serial.print(data.conductivity); Serial.print("us/cm\t");
        Serial.print("pH: "); Serial.print(data.ph); Serial.print("\t");
        Serial.print("N: "); Serial.print(data.nitrogen); Serial.print("mg/kg\t");
        Serial.print("P: "); Serial.print(data.phosphorus); Serial.print("mg/kg\t");
        Serial.print("K: "); Serial.print(data.potassium); Serial.println("mg/kg");
    } else {
        Serial.println("Failed to read from sensor!");
    }
    return state;
}

void setup() {
    DDRB |= _BV(DDB5); // Set LED pin as output

    Serial.begin(9600);
    mySerial.begin(9600);
    gSensor.begin(mySerial, 9600);

    Serial.println("Soil Sensor Test");

    scheduler_init(tasks, TOTAL_TASKS_NUM);
    TimerSet();
}

void loop() {
    // Idle; scheduler runs from Timer1 ISR
}