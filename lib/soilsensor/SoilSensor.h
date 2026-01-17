#ifndef SOIL_SENSOR_LIB_H
#define SOIL_SENSOR_LIB_H

#define SOIL_SENSOR_DEBUG

#include <ModbusMaster.h>

// From the PDF, the registers are:
// 0x0000: Moisture
// 0x0001: Temperature
// 0x0002: Conductivity
// 0x0003: pH
// 0x0004: Nitrogen (N)
// 0x0005: Phosphorus (P)
// 0x0006: Potassium (K)

#define SOIL_MOISTURE_REG      0x0012
#define SOIL_TEMPERATURE_REG   0x0013
#define SOIL_CONDUCTIVITY_REG  0x0015
#define SOIL_PH_REG            0x0006
#define SOIL_NITROGEN_REG      0x001E
#define SOIL_PHOSPHORUS_REG    0x001F
#define SOIL_POTASSIUM_REG     0x0020

#define SOIL_DEVICE_ADDRESS_REG 0x0100
#define SOIL_BAUD_RATE_REG      0x0101


class SoilSensor {
public:
    struct SensorData {
        float moisture;
        float temperature;
        uint16_t conductivity;
        float ph;
        uint16_t nitrogen;
        uint16_t phosphorus;
        uint16_t potassium;
    };

    SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin);
    void begin(Stream &serial, long baud);
    
    bool readAll(SensorData &data);
    float readMoisture();
    float readTemperature();
    uint16_t readConductivity();
    float readPH();
    uint16_t readNitrogen();
    uint16_t readPhosphorus();
    uint16_t readPotassium();

    bool setDeviceAddress(uint8_t newAddress);
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
