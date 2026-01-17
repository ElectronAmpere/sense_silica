/**
 * @file SoilSensor.h
 * @author VigneshRR
 * @brief Developer-level documentation for an Arduino library for the JXBS-3001-NPK-RS 7-in-1 Soil Sensor.
 * 
 * @details This library provides a high-level interface to read data from the JXBS-3001 series soil sensors
 * using the Modbus RTU protocol. It abstracts the underlying Modbus communication and the specific
 * register mappings for the sensor. The library is designed to work with the ModbusMaster library
 * and handles the necessary RS485 direction control (DE/RE pins).
 * 
 * @version 0.2
 * @date 2026-01-17
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef SOIL_SENSOR_LIB_H
#define SOIL_SENSOR_LIB_H

#include <ModbusMaster.h>

// Register addresses for the JXBS-3001-NPK-RS sensor.
// These values are based on the official user manual. Note that different sensor
// versions might have different register maps. These are confirmed for V2.0.
#define SOIL_MOISTURE_REG      0x0012 ///< Modbus register for soil moisture.
#define SOIL_TEMPERATURE_REG   0x0013 ///< Modbus register for soil temperature.
#define SOIL_CONDUCTIVITY_REG  0x0015 ///< Modbus register for soil conductivity.
#define SOIL_PH_REG            0x0006 ///< Modbus register for soil pH.
#define SOIL_NITROGEN_REG      0x001E ///< Modbus register for soil nitrogen.
#define SOIL_PHOSPHORUS_REG    0x001F ///< Modbus register for soil phosphorus.
#define SOIL_POTASSIUM_REG     0x0020 ///< Modbus register for soil potassium.

// Configuration registers
#define SOIL_DEVICE_ADDRESS_REG 0x0100 ///< Modbus register to change the device's slave address.
#define SOIL_BAUD_RATE_REG      0x0101 ///< Modbus register to change the device's baud rate.

/**
 * @class SoilSensor
 * @brief Implements the logic for interacting with the JXBS-3001-NPK-RS soil sensor.
 * @details This class encapsulates Modbus communication, RS485 direction control,
 * and data parsing for the sensor. It relies on an external ModbusMaster instance.
 */
class SoilSensor {
public:
    /**
     * @struct SensorData
     * @brief A structure to hold a complete snapshot of all sensor readings.
     * @details All floating-point values are scaled from the raw integer values
     * received from the sensor as per the device manual (typically by a factor of 10).
     */
    struct SensorData {
        float moisture;      ///< Soil moisture content in %RH. Scaled from a raw integer value.
        float temperature;   ///< Soil temperature in degrees Celsius (°C). Scaled from a raw integer value.
        uint16_t conductivity; ///< Soil electrical conductivity (EC) in microsiemens per centimeter (µS/cm).
        float ph;            ///< Soil pH value. Scaled from a raw integer value.
        uint16_t nitrogen;    ///< Nitrogen (N) content in milligrams per kilogram (mg/kg).
        uint16_t phosphorus;  ///< Phosphorus (P) content in milligrams per kilogram (mg/kg).
        uint16_t potassium;   ///< Potassium (K) content in milligrams per kilogram (mg/kg).
    };

    /**
     * @brief Construct a new Soil Sensor object.
     * @details This constructor initializes the sensor object with a reference to a
     * ModbusMaster node and the GPIO pins used for RS485 direction control.
     * 
     * @param node A reference to an initialized ModbusMaster object. This library will use this object to perform Modbus transactions.
     * @param rePin The GPIO pin number connected to the Receiver Enable (RE) pin of the MAX485 or similar transceiver.
     * @param dePin The GPIO pin number connected to the Driver Enable (DE) pin of the MAX485 or similar transceiver.
     */
    SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin);

    /**
     * @brief Initializes the hardware and software components for sensor communication.
     * @details This method sets up the ModbusMaster instance with a default slave ID,
     * configures the direction control pins, and registers the pre/post-transmission
     * callbacks required for half-duplex RS485 communication.
     * 
     * @param serial A reference to the Stream object (e.g., SoftwareSerial or HardwareSerial) used for communication.
     * @param baud The initial baud rate for the serial communication. This should match the sensor's configured baud rate.
     */
    void begin(Stream &serial, long baud);
    
    /**
     * @brief Reads all primary sensor values in a series of Modbus transactions.
     * @details This function is a convenience wrapper that reads pH, moisture, temperature,
     * conductivity, and NPK values. It performs multiple separate Modbus read requests
     * because the required registers are not in a single contiguous block.
     * 
     * @param data A reference to a SensorData struct where the successfully read values will be stored.
     * @return Returns `true` if all Modbus transactions were successful, `false` if any one of them fails.
     */
    bool readAll(SensorData &data);

    /**
     * @brief Reads only the soil moisture value.
     * @details Performs a single Modbus transaction to read the moisture register.
     * @return The soil moisture in %RH, or -1.0 on a communication error.
     */
    float readMoisture();

    /**
     * @brief Reads only the soil temperature value.
     * @details Performs a single Modbus transaction to read the temperature register.
     * @return The soil temperature in °C, or -999.0 on a communication error.
     */
    float readTemperature();

    /**
     * @brief Reads only the soil conductivity value.
     * @details Performs a single Modbus transaction to read the conductivity register.
     * @return The soil conductivity in µS/cm, or 0xFFFF (65535) on a communication error.
     */
    uint16_t readConductivity();

    /**
     * @brief Reads only the soil pH value.
     * @details Performs a single Modbus transaction to read the pH register.
     * @return The soil pH value, or -1.0 on a communication error.
     */
    float readPH();

    /**
     * @brief Reads only the nitrogen (N) content.
     * @details Performs a single Modbus transaction to read the nitrogen register.
     * @return The nitrogen content in mg/kg, or 0xFFFF (65535) on a communication error.
     */
    uint16_t readNitrogen();

    /**
     * @brief Reads only the phosphorus (P) content.
     * @details Performs a single Modbus transaction to read the phosphorus register.
     * @return The phosphorus content in mg/kg, or 0xFFFF (65535) on a communication error.
     */
    uint16_t readPhosphorus();

    /**
     * @brief Reads only the potassium (K) content.
     * @details Performs a single Modbus transaction to read the potassium register.
     * @return The potassium content in mg/kg, or 0xFFFF (65535) on a communication error.
     */
    uint16_t readPotassium();

    /**
     * @brief Changes the Modbus slave address of the sensor.
     * @warning This is a permanent configuration change written to the sensor's non-volatile memory.
     * After a successful change, the library automatically updates the ModbusMaster instance to use the new address for subsequent requests.
     * 
     * @param newAddress The new slave address (1-247).
     * @return Returns `true` if the write transaction was successful, `false` otherwise.
     */
    bool setDeviceAddress(uint8_t newAddress);

    /**
     * @brief Changes the communication baud rate of the sensor.
     * @warning This is a permanent configuration change. After calling this, you must
     * re-initialize your local serial port to match the new baud rate.
     * 
     * @param baudRateCode The code for the new baud rate: 1 for 2400, 2 for 4800, 3 for 9600.
     * @return Returns `true` if the write transaction was successful, `false` otherwise.
     */
    bool setBaudRate(uint16_t baudRateCode);

private:
    ModbusMaster& _node;      ///< A reference to the underlying ModbusMaster object.
    Stream*       _serial;    ///< A pointer to the serial stream, stored for re-initializing ModbusMaster on address change.
    uint8_t       _rePin;     ///< GPIO pin for Receiver Enable on the RS485 transceiver.
    uint8_t       _dePin;     ///< GPIO pin for Driver Enable on the RS485 transceiver.

    /**
     * @brief Internal helper function to read a single 16-bit holding register.
     * @param reg The address of the register to read.
     * @return The 16-bit value of the register, or 0xFFFF on error.
     */
    uint16_t getRegisterValue(uint16_t reg);

    /**
     * @brief Static callback function executed before a Modbus transmission.
     * @details This function is registered with ModbusMaster to handle RS485 direction control.
     * It sets the RE and DE pins to enable the transmitter. It uses a static instance
     * pointer to access the object's pins.
     */
    static void preTransmission();

    /**
     * @brief Static callback function executed after a Modbus transmission.
     * @details This function is registered with ModbusMaster to handle RS485 direction control.
     * It flushes the serial buffer and then sets the RE and DE pins to enable the receiver.
     */
    static void postTransmission();

    /**
     * @brief A static pointer to the current instance of the class.
     * @details This is a workaround to allow the static pre/post-transmission callback
     * functions to access the instance-specific members (like pin numbers and the serial stream).
     * This assumes only one instance of the SoilSensor class is active.
     */
    static SoilSensor* _instance;
};

#endif // SOIL_SENSOR_LIB_H
