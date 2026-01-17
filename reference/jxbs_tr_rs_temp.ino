/**
 * JXBS-3001-NPK-RS Soil NPK Sensor User Manual
 * Example code for reading data from the sensor via RS485
 */

#include <SoftwareSerial.h>
#include <Wire.h>

#define RE_PIN 8
#define DE_PIN 7

/** Inquiry frame */
const byte soiltemp_humid_npk_inquiry[] = {
  /** Address Code (1), Functional Code (1), Register Starting Address (2), Register Length (2), CRC LowByte (1), CRC HighByte (1) */
  0x01, 0x03, 0x00, 0x12, 0x00, 0x02, 0x64, 0x0E
};

SoftwareSerial rs485Serial(2, 3); // RX, TX
byte response[11]; // Response buffer

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(9600);

  pinMode(RE_PIN, OUTPUT);
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(RE_PIN, LOW);
  digitalWrite(DE_PIN, LOW);

  delay(500); // Wait for sensor to initialize
}

byte getSoilTemperatureHumidityNPK() {
  digitalWrite(DE_PIN, HIGH); // Enable transmission
  digitalWrite(RE_PIN, HIGH); // Enable reception
  delay(10);

  if (rs485Serial.write(soiltemp_humid_npk_inquiry, sizeof(soiltemp_humid_npk_inquiry)) != sizeof(soiltemp_humid_npk_inquiry)) {
    Serial.println("Failed to send inquiry frame");
    digitalWrite(DE_PIN, LOW);
    digitalWrite(RE_PIN, LOW);
    return 0xFF;
  } else {
    digitalWrite(DE_PIN, LOW);
    digitalWrite(RE_PIN, LOW);
    for (byte index = 0; index < 11; index++) {
      response[index] = rs485Serial.read();
    }
    return response[4]; // Return soil temperature and humidity byte
  }
}

void loop() {
  byte soilTempHumid = getSoilTemperatureHumidityNPK();
  delay(250);
  if (soilTempHumid != 0xFF) {
    byte soilTemperature = (soilTempHumid >> 4) & 0x0F; // Upper 4 bits
    byte soilHumidity    = soilTempHumid & 0x0F;        // Lower 4 bits

    Serial.print("Soil Temperature: ");
    Serial.print(soilTemperature);
    Serial.println(" °C");

    Serial.print("Soil Humidity: ");
    Serial.print(soilHumidity);
    Serial.println(" %");
  } else {
    Serial.println("Error reading soil temperature and humidity");
  }

  delay(2000); // Wait before next reading
}