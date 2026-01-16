/**
 * Simple Blink Example for RIoS
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
 * https://www.cs.ucr.edu/~vahid/rios/
 * https://www.cs.ucr.edu/~vahid/pes/RITools/ 
 */
/** Generic library */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>


// Task type and scheduler API
#include "config.h"
#include "scheduler.h"

/** Custom library */
#include "timer.h"
#include "modbus_rtu.h"
#include "rs485.h"
#include "modbus_registers.h"
#include "modbus_client.h"
#include "soil_sensor.h"

#define RE_PIN  8
#define DE_PIN  9

int Task_ToggleLED(int state);
int Task_SoilSensor(int state);

void TimerSet(void)
{
    // Auto-reload via CTC at TASK_TICKS_GCD_IN_MS
    timer1_set_period_ms(TASK_TICKS_GCD_IN_MS);
    sei(); // Enable interrupt
}

static task tasks[TOTAL_TASKS_NUM] = {
    {0, 0, 100, 0, &Task_ToggleLED}, // Toggle LED every 100 ms
    {0, 0, 1000, 0, &Task_SoilSensor} // Read soil sensor every 1000 ms
};
    
SoftwareSerial mySerial(2, 3); // RX, TX
static ModbusClientConfig gModbusClient;
static SoilSensor gSensor;

int Task_ToggleLED(int state) {
    switch(state) {
        case 0:
            // Toggle LED on PB5
            PORTB ^= _BV(PORTB5);
            state = 0;
            break;
        default:
            state = 0;
            break;
    }
    return state;
}

// Modbus RTU helpers for JXBS-3001-NPK-RS
static void print_all_values();

int Task_SoilSensor(int state) {
    switch(state) {
        case 0:
            // Read and display all primary values from sensor
            print_all_values();
            state = 0;
            break;
        default:
            state = 0;
            break;
    }
    return state;
}

// (legacy nitro/values removed; using generic Modbus helper)

// --- Modbus RTU helpers for JXBS-3001-NPK-RS ---
static void print_all_values() {
    SoilData d;
    if (!soil_sensor_read_all(&gSensor, &d)) {
        Serial.println("Sensor read failed");
        return;
    }
    Serial.print("pH: "); Serial.print(d.ph, 2);
    Serial.print(" | RH (%): "); Serial.print(d.moisturePct, 1);
    Serial.print(" | Temp (C): "); Serial.print(d.temperatureC, 1);
    Serial.print(" | EC (uS/cm): "); Serial.print(d.conductivity);
    Serial.print(" | N (mg/kg): "); Serial.print(d.nitrogen);
    Serial.print(" | P (mg/kg): "); Serial.print(d.phosphorus);
    Serial.print(" | K (mg/kg): "); Serial.println(d.potassium);
}

// (removed legacy getSoilNitrogenLevel; using print_npk_values in task)

int main() {

    // Set PB5 as output without clobbering other pins
    DDRB |= _BV(DDB5);

    mySerial.begin(NPK_RS485_DEFAULT.baudRate);
    Serial.begin(9600);
    Serial.println("RIoS Blink Example");

    gModbusClient.io = &mySerial;
    gModbusClient.rePin = RE_PIN;
    gModbusClient.dePin = DE_PIN;
    gModbusClient.rs485 = NPK_RS485_DEFAULT;
    gModbusClient.timeoutMs = 200;
    gModbusClient.maxRetries = 2;
    modbus_client_init(&gModbusClient);

    soil_sensor_init(&gSensor, &gModbusClient, 0x01);

    // Initialize scheduler with tasks
    scheduler_init(tasks, TOTAL_TASKS_NUM);

    TimerSet();

    while(1)
    {
        // Do anything, this timer is non-blocking. It will interrupt the CPU only when needed
    }
    
    // Add return so old compilers don't cry about it being missing. Under normal circumstances this will never be hit
    return 0;
}