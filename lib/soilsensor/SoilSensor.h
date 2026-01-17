#ifndef SOIL_SENSOR_LIB_H
#define SOIL_SENSOR_LIB_H

#include <ModbusMaster.h>
#include <stdint.h>

// JSF AV C++ Rule 10: Use constexpr for constants.
namespace sensor_registers {
    constexpr uint16_t SOIL_MOISTURE_REG = 0x0012;
    constexpr uint16_t SOIL_TEMPERATURE_REG = 0x0013;
    constexpr uint16_t SOIL_CONDUCTIVITY_REG = 0x0015;
    constexpr uint16_t SOIL_PH_REG = 0x0006;
    constexpr uint16_t SOIL_NITROGEN_REG = 0x001E;
    constexpr uint16_t SOIL_PHOSPHORUS_REG = 0x001F;
    constexpr uint16_t SOIL_POTASSIUM_REG = 0x0020;
    constexpr uint16_t SOIL_DEVICE_ADDRESS_REG = 0x0100;
    constexpr uint16_t SOIL_BAUD_RATE_REG = 0x0101;
}

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

    // JSF AV C++ Rule 39: explicit constructor.
    explicit SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin) noexcept;

    // JSF AV C++ Rule 30, 32: Prohibit copy construction and assignment.
    SoilSensor(const SoilSensor&) = delete;
    SoilSensor& operator=(const SoilSensor&) = delete;
    ~SoilSensor() = default;

    void begin(Stream &serial, long baud) noexcept;
    
    bool readAll(SensorData &data) noexcept;

    float readMoisture() noexcept;
    float readTemperature() noexcept;
    uint16_t readConductivity() noexcept;
    float readPH() noexcept;
    uint16_t readNitrogen() noexcept;
    uint16_t readPhosphorus() noexcept;
    uint16_t readPotassium() noexcept;

    bool setDeviceAddress(uint8_t newAddress) noexcept;
    bool setBaudRate(uint16_t baudRateCode) noexcept;

private:
    // JSF AV C++ Rule 23: All data members shall be private.
    ModbusMaster& _node;
    Stream*       _serial;
    const uint8_t _rePin;
    const uint8_t _dePin;

    uint16_t getRegisterValue(uint16_t reg) noexcept;

    // JSF AV C++ Rule 12: static for file scope.
    static void preTransmission() noexcept;
    static void postTransmission() noexcept;

    // JSF AV C++ Rule 70: No volatile. Singleton pattern avoids it.
    static SoilSensor* _instance;
};

#endif // SOIL_SENSOR_LIB_H
