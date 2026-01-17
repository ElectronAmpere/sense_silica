/**
 * @file SoilSensor.h
 * @author your name
 * @brief Arduino library for the JXBS-3001-NPK-RS 7-in-1 Soil Sensor.
 * 
 * This library provides a simple interface to read data from the soil sensor
 * using Modbus RTU communication. It handles the RS485 communication nuances
 * and provides methods to read all sensor values.
 * 
 * @version 0.1
 * @date 2026-01-17
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef SOIL_SENSOR_LIB_H
#define SOIL_SENSOR_LIB_H

#include <ModbusMaster.h>

// Based on the JXBS-3001-NPK-RS sensor documentation.
#define SOIL_MOISTURE_REG      0x0012
#define SOIL_TEMPERATURE_REG   0x0013
#define SOIL_CONDUCTIVITY_REG  0x0015
#define SOIL_PH_REG            0x0006
#define SOIL_NITROGEN_REG      0x001E
#define SOIL_PHOSPHORUS_REG    0x001F
#define SOIL_POTASSIUM_REG     0x0020

#define SOIL_DEVICE_ADDRESS_REG 0x0100
#define SOIL_BAUD_RATE_REG      0x0101

/**
 * @class SoilSensor
 * @brief Class to interact with the JXBS-3001-NPK-RS soil sensor.
 */
class SoilSensor {
public:
    /**
     * @struct SensorData
     * @brief Holds all the data read from the soil sensor.
     */
    struct SensorData {
        float moisture;      ///< Soil moisture in %
        float temperature;   ///< Soil temperature in °C
        uint16_t conductivity; ///< Soil conductivity in µS/cm
        float ph;            ///< Soil pH
        uint16_t nitrogen;    ///< Nitrogen content in mg/kg
        uint16_t phosphorus;  ///< Phosphorus content in mg/kg
        uint16_t potassium;   ///< Potassium content in mg/kg
    };

    /**
     * @brief Construct a new SoilSensor object.
     * 
     * @param node A reference to the ModbusMaster object.
     * @param rePin The Receiver Enable (RE) pin for the RS485 transceiver.
     * @param dePin The Driver Enable (DE) pin for the RS485 transceiver.
     */
    SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin);

    /**
     * @brief Initializes the sensor and the Modbus communication.
     * 
     * @param serial A reference to the Stream object for communication (e.g., SoftwareSerial).
     * @param baud The baud rate for the serial communication.
     */
    void begin(Stream &serial, long baud);
    
    /**
     * @brief Reads all sensor values in one go.
     * 
     * @param data A reference to a SensorData struct to store the read values.
     * @return true if the read was successful, false otherwise.
     */
    bool readAll(SensorData &data);

    /**
     * @brief Reads the soil moisture.
     * @return The soil moisture in %, or -1.0 on error.
     */
    float readMoisture();

    /**
     * @brief Reads the soil temperature.
     * @return The soil temperature in °C, or -999.0 on error.
     */
    float readTemperature();

    /**
     * @brief Reads the soil conductivity.
     * @return The soil conductivity in µS/cm, or 0xFFFF on error.
     */
    uint16_t readConductivity();

    /**
     * @brief Reads the soil pH.
     * @return The soil pH, or -1.0 on error.
     */
    float readPH();

    /**
     * @brief Reads the nitrogen content.
     * @return The nitrogen content in mg/kg, or 0xFFFF on error.
     */
    uint16_t readNitrogen();

    /**
     * @brief Reads the phosphorus content.
     * @return The phosphorus content in mg/kg, or 0xFFFF on error.
     */
    uint16_t readPhosphorus();

    /**
     * @brief Reads the potassium content.
     * @return The potassium content in mg/kg, or 0xFFFF on error.
     */
    uint16_t readPotassium();

    /**
     * @brief Sets the Modbus device address of the sensor.
     * 
     * @param newAddress The new device address.
     * @return true if the address was set successfully, false otherwise.
     */
    bool setDeviceAddress(uint8_t newAddress);

    /**
     * @brief Sets the baud rate of the sensor.
     * 
     * @param baudRateCode The code for the new baud rate (1=2400, 2=4800, 3=9600).
     * @return true if the baud rate was set successfully, false otherwise.
     */
    bool setBaudRate(uint16_t baudRateCode);

private:
    ModbusMaster &_node;
    Stream* _serial;
    uint8_t _rePin;
    uint8_t _dePin;

    uint16_t getRegisterValue(uint16_t reg);
    static void preTransmission();
    static void postTransmission();

    static SoilSensor* _instance;
};

#endif // SOIL_SENSOR_LIB_H
