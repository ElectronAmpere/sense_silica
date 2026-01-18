#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include "SoilSensor.h"
#include "lcd.h"
#include "config.h"
#include "scheduler.h"
#include "timer.h"

// JSF AV C++ Rule 90: Avoid magic numbers.
namespace {
    constexpr uint8_t RE_PIN = 8;
    constexpr uint8_t DE_PIN = 7;
    constexpr uint8_t LED_PIN_B5 = 5;
    constexpr long SERIAL_BAUD_RATE = 9600;
    constexpr uint32_t LED_TOGGLE_PERIOD_MS = 100;
    constexpr uint32_t SENSOR_READ_PERIOD_MS = 2000;
    constexpr uint32_t LCD_UPDATE_PERIOD_MS = 2000;
}

// Forward declarations for task functions
int Task_ToggleLED(int state);
int Task_SoilSensor(int state);
int Task_LcdUpdate(int state);

// Static initializations
SoftwareSerial mySerial(2, 3); // RX, TX
ModbusMaster node;
SoilSensor gSensor(node, RE_PIN, DE_PIN);
// Define pins (adjust as needed; ensure no conflicts with RS485 or other peripherals)
LCD lcd(12, 11, 5, 4, 3, 2, 10);  // RS, EN, D4-D7, Backlight (PWM pin)
 SoilSensor::SensorData data;

// JSF AV C++ Rule 18: All variables shall be initialized.
avr_embedded::Task tasks[avr_embedded::TOTAL_TASKS_NUM] = {
    avr_embedded::Task(LED_TOGGLE_PERIOD_MS, &Task_ToggleLED),
    avr_embedded::Task(SENSOR_READ_PERIOD_MS, &Task_SoilSensor),
    avr_embedded::Task(LCD_UPDATE_PERIOD_MS, &Task_LcdUpdate)
};

int Task_ToggleLED(int state) {
    // JSF AV C++ Rule 144: Direct use of bitwise operators is restricted.
    // Using Arduino API for clarity, though direct port manipulation is efficient.
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    return state; // State is not used in this task
}

int Task_SoilSensor(int state) {
    if (gSensor.readAll(data)) {
        // JSF AV C++ Rule 20: Floating-point constants should have a decimal.
        Serial.print("Moisture: "); Serial.print(data.moisture, 2); Serial.print("%\t");
        Serial.print("Temperature: "); Serial.print(data.temperature, 2); Serial.print("C\t");
        Serial.print("Conductivity: "); Serial.print(data.conductivity); Serial.print("us/cm\t");
        Serial.print("pH: "); Serial.print(data.ph, 2); Serial.print("\t");
        Serial.print("N: "); Serial.print(data.nitrogen); Serial.print("mg/kg\t");
        Serial.print("P: "); Serial.print(data.phosphorus); Serial.print("mg/kg\t");
        Serial.print("K: "); Serial.print(data.potassium); Serial.println("mg/kg");
    } else {
        Serial.println("Failed to read from sensor!");
    }
    return state; // State is not used in this task
}

int Task_LcdUpdate(int state) {
    // Assuming data is a struct with float temperature and float ph
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(data.temperature, 1);  // 1 decimal place (adjust as needed)
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("pH:   ");
    lcd.print(data.ph, 2);           // 2 decimal places for pH
    return state;
}

int main(void) {
    // Manually call the Arduino core init function.
    // This sets up timers and other hardware required by Arduino APIs.
    init();

    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(SERIAL_BAUD_RATE);
    mySerial.begin(SERIAL_BAUD_RATE);
    gSensor.begin(mySerial, SERIAL_BAUD_RATE);
    lcd.begin();
    lcd.print("Soil Sensor Init");

    Serial.println("Soil Sensor Test - JSF Compliant Version");

    scheduler_init(tasks, avr_embedded::TOTAL_TASKS_NUM);
    timer1_set_period_ms(avr_embedded::TASK_TICKS_GCD_IN_MS);
    
    // Enable global interrupts
    sei();

    // Infinite loop to keep the program running.
    // All tasks are handled by the timer-driven scheduler.
    while (true) {
        // The main loop can be used for very low-priority tasks
        // or be put into a sleep mode to save power.
    }

    return 0; // This line will never be reached.
}