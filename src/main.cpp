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
    {0, 0, 1000, 0, &Task_ToggleLED}, // Toggle LED every 100 ms
    {0, 0, 100, 0, &Task_SoilSensor} // Read soil sensor every 1000 ms
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

byte getSoilNitrogenLevel(void);

int Task_SoilSensor(int state) {
    byte nitrogen = 0;
    switch(state) {
        case 0:
            // Placeholder for soil sensor reading logic
            state = 0;
            nitrogen = getSoilNitrogenLevel(); // Hypothetical function
             Serial.print("Soil Nitrogen Level: ");
             Serial.println(nitrogen);
            break;
        default:
            state = 0;
            break;
    }
    return state;
}

// Modbus RTU requests for reading NPK values
const byte nitro[] = { 0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c };

// A variable used to store NPK values
byte values[11];

byte getSoilNitrogenLevel(void) {
    digitalWrite(RE_PIN, HIGH);
    digitalWrite(DE_PIN, HIGH);
    _delay_ms(10); // Wait for the transceiver to stabilize

    if (mySerial.write(nitro, sizeof(nitro)) == 8) {
        digitalWrite(DE_PIN, LOW);
        digitalWrite(RE_PIN, LOW);
        for (byte i = 0; i < 7; i++) {
            values[i] = mySerial.read();
            Serial.print(values[i], HEX);
        }
        Serial.println();
    }
    return values[4];
}

int main() {

    // Set PB5 as output without clobbering other pins
    DDRB |= _BV(DDB5);

    mySerial.begin(9600);
    Serial.begin(9600);
    Serial.println("RIoS Blink Example");

    pinMode(RE_PIN, OUTPUT);
    pinMode(DE_PIN, OUTPUT);

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