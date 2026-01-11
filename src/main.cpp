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
static uint16_t modbus_crc16(const uint8_t* data, size_t len);
static bool rs485_read_register(uint8_t slaveId, uint16_t regAddr, uint16_t* outValue);
static void print_npk_values();

int Task_SoilSensor(int state) {
    switch(state) {
        case 0:
            // Read and display all NPK values from sensor
            print_npk_values();
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
static uint16_t modbus_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i];
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static bool rs485_read_register(uint8_t slaveId, uint16_t regAddr, uint16_t* outValue) {
    uint8_t frame[8];
    frame[0] = slaveId;
    frame[1] = 0x03; // Read Holding Registers
    frame[2] = (uint8_t)(regAddr >> 8);
    frame[3] = (uint8_t)(regAddr & 0xFF);
    frame[4] = 0x00;
    frame[5] = 0x01; // read 1 register
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);       // CRC low
    frame[7] = (uint8_t)((crc >> 8) & 0xFF); // CRC high

    // Transmit request
    digitalWrite(RE_PIN, HIGH);
    digitalWrite(DE_PIN, HIGH);
    delayMicroseconds(300);
    size_t written = mySerial.write(frame, sizeof(frame));
    if (written != sizeof(frame)) {
        // Ensure we return to receive mode even on error
        digitalWrite(DE_PIN, LOW);
        digitalWrite(RE_PIN, LOW);
        return false;
    }
    // Switch to receive
    digitalWrite(DE_PIN, LOW);
    digitalWrite(RE_PIN, LOW);

    // Expect 7-byte response: id, func, bytecount(2), data_hi, data_lo, crc_lo, crc_hi
    uint8_t resp[7];
    uint8_t idx = 0;
    const uint32_t timeout_us = 200000UL; // 200 ms
    uint32_t waited = 0;
    while (idx < 7 && waited < timeout_us) {
        if (mySerial.available()) {
            int c = mySerial.read();
            if (c >= 0) {
                resp[idx++] = (uint8_t)c;
            }
        } else {
            delayMicroseconds(100);
            waited += 100;
        }
    }
    if (idx < 7) {
        return false; // timeout
    }

    // Validate response
    if (resp[0] != slaveId || resp[1] != 0x03 || resp[2] != 0x02) {
        return false;
    }
    uint16_t respCrcCalc = modbus_crc16(resp, 5);
    uint16_t respCrc = (uint16_t)resp[5] | ((uint16_t)resp[6] << 8);
    if (respCrcCalc != respCrc) {
        return false;
    }

    *outValue = ((uint16_t)resp[3] << 8) | (uint16_t)resp[4];
    return true;
}

static void print_npk_values() {
    const uint8_t slaveId = 0x01;
    // JXBS-3001-NPK-RS registers: 0x001E (N), 0x001F (P), 0x0020 (K)
    uint16_t n = 0, p = 0, k = 0;
    bool okN = rs485_read_register(slaveId, 0x001E, &n);
    bool okP = rs485_read_register(slaveId, 0x001F, &p);
    bool okK = rs485_read_register(slaveId, 0x0020, &k);

    Serial.print("N (mg/kg): "); Serial.print(okN ? n : 0);
    Serial.print(" | P (mg/kg): "); Serial.print(okP ? p : 0);
    Serial.print(" | K (mg/kg): "); Serial.println(okK ? k : 0);
}

// (removed legacy getSoilNitrogenLevel; using print_npk_values in task)

int main() {

    // Set PB5 as output without clobbering other pins
    DDRB |= _BV(DDB5);

    mySerial.begin(9600);
    Serial.begin(9600);
    Serial.println("RIoS Blink Example");

    pinMode(RE_PIN, OUTPUT);
    pinMode(DE_PIN, OUTPUT);
    // Default to receive mode
    digitalWrite(RE_PIN, LOW);
    digitalWrite(DE_PIN, LOW);

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