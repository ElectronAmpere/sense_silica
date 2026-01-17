/**
 * Simple Blink + Sensor Scheduler using RIoS-style cooperative tasks
 * https://www.cs.ucr.edu/~vahid/pubs/wese12_rios.pdf
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
#include "logger.h"

#define RE_PIN  8
#define DE_PIN  7

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
static bool gDidRawDump = false;

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
            if (!gDidRawDump) {
                gDidRawDump = true;
                uint8_t req[8];
                modbus_rtu_build_read_request(0x01, 0x001E, 1, req);
                uint8_t buf[32]; uint8_t len = 0;
                (void)modbus_client_request_raw(&gModbusClient, req, sizeof(req), buf, sizeof(buf), &len);
                Serial.print("RAW resp ("); Serial.print(len); Serial.println(") bytes:");
                for (uint8_t i = 0; i < len; ++i) { Serial.print(buf[i], HEX); Serial.print(' '); }
                Serial.println();

                // Also dump moisture+temperature (0x0012, qty=2)
                modbus_rtu_build_read_request(0x01, 0x0012, 2, req);
                len = 0;
                (void)modbus_client_request_raw(&gModbusClient, req, sizeof(req), buf, sizeof(buf), &len);
                Serial.print("RAW resp MT ("); Serial.print(len); Serial.println(") bytes:");
                for (uint8_t i = 0; i < len; ++i) { Serial.print(buf[i], HEX); Serial.print(' '); }
                Serial.println();
            }
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

// Arduino setup/loop entry points
void setup() {
    // Set PB5 as output without clobbering other pins
    DDRB |= _BV(DDB5);

    mySerial.begin(NPK_RS485_DEFAULT.baudRate);
    Serial.begin(9600);
    Serial.println("RIoS Blink + Sensor Scheduler");
    // Give the monitor a moment to attach before probe logs
    delay(300);

    gModbusClient.io = &mySerial;
    gModbusClient.rePin = RE_PIN;
    gModbusClient.dePin = DE_PIN;
    gModbusClient.reActiveLow = true;   // MAX485 default
    gModbusClient.deActiveHigh = true;  // MAX485 default
    gModbusClient.rs485 = NPK_RS485_DEFAULT;
    gModbusClient.timeoutMs = 500;
    gModbusClient.maxRetries = 2;
    gModbusClient.trace = true; // enable detailed tracing
    modbus_client_init(&gModbusClient);

    // Probe for sensor across common baud rates and addresses
    const uint32_t bauds[] = {9600, 4800, 2400};
    const uint8_t addrs[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t foundAddr = 0;
    uint32_t foundBaud = 0;
    for (uint8_t bi = 0; bi < sizeof(bauds)/sizeof(bauds[0]); ++bi) {
        foundAddr = 0; foundBaud = bauds[bi];
        mySerial.begin(foundBaud);
        // Tune timeout per baud (slower baud → longer timeout)
        if (foundBaud >= 9600) gModbusClient.timeoutMs = 500;
        else if (foundBaud >= 4800) gModbusClient.timeoutMs = 700;
        else gModbusClient.timeoutMs = 900;
        gModbusClient.rs485.baudRate = foundBaud;
        modbus_client_init(&gModbusClient);
        Serial.print("Probing baud "); Serial.print(foundBaud);
        Serial.print(" addresses: ");
        for (uint8_t i = 0; i < (uint8_t)(sizeof(addrs)/sizeof(addrs[0])); ++i) { Serial.print(addrs[i], HEX); Serial.print(' '); }
        Serial.println();
        if (modbus_client_probe_addresses(&gModbusClient, addrs, sizeof(addrs), &foundAddr)) {
            break;
        }
    }
    if (foundAddr == 0) {
        Serial.println("Probe failed: defaulting to addr 0x01 @ 9600");
        foundAddr = 0x01;
        foundBaud = 9600;
        mySerial.begin(foundBaud);
        gModbusClient.rs485.baudRate = foundBaud;
        gModbusClient.timeoutMs = 500;
        modbus_client_init(&gModbusClient);
    } else {
        Serial.print("Probe success: addr="); Serial.print(foundAddr, HEX);
        Serial.print(" baud="); Serial.println(foundBaud);
    }

    soil_sensor_init(&gSensor, &gModbusClient, foundAddr);

    // Initialize scheduler with tasks
    scheduler_init(tasks, TOTAL_TASKS_NUM);

    TimerSet();
}

void loop() {
    // Idle; scheduler runs from Timer1 ISR
}