#include "SoilSensor.h"

// JSF AV C++ Rule 12: static for file scope.
SoilSensor* SoilSensor::_instance = nullptr;

SoilSensor::SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin) noexcept
    : _node(node), _serial(nullptr), _rePin(rePin), _dePin(dePin) {
    _instance = this;
}

void SoilSensor::begin(Stream &serial, long) noexcept {
    _serial = &serial;
    // JSF AV C++ Rule 18: Initialize all variables.
    constexpr uint8_t default_slave_id = 1;
    _node.begin(default_slave_id, *_serial);
    
    pinMode(_rePin, OUTPUT);
    pinMode(_dePin, OUTPUT);
    
    digitalWrite(_dePin, LOW);
    digitalWrite(_rePin, LOW);
    
    _node.preTransmission(preTransmission);
    _node.postTransmission(postTransmission);
}

void SoilSensor::preTransmission() noexcept {
    if (_instance != nullptr) {
        digitalWrite(_instance->_rePin, HIGH);
        digitalWrite(_instance->_dePin, HIGH);
        // JSF AV C++ Rule 208: The use of delay functions is prohibited.
        // This delay is a hardware necessity for the RS485 transceiver to switch modes.
        // A better implementation would use a timer or hardware signal.
        delay(10);
    }
}

void SoilSensor::postTransmission() noexcept {
    if (_instance != nullptr && _instance->_serial != nullptr) {
        _instance->_serial->flush();
        digitalWrite(_instance->_dePin, LOW);
        digitalWrite(_instance->_rePin, LOW);
    }
}

uint16_t SoilSensor::getRegisterValue(uint16_t reg) noexcept {
    const uint8_t result = _node.readHoldingRegisters(reg, 1);
    if (result == ModbusMaster::ku8MBSuccess) {
        return _node.getResponseBuffer(0);
    }
    return 0xFFFF; // Error
}

float SoilSensor::readMoisture() noexcept {
    const uint16_t val = getRegisterValue(sensor_registers::SOIL_MOISTURE_REG);
    return val != 0xFFFF ? static_cast<float>(val) / 10.0f : -1.0f;
}

float SoilSensor::readTemperature() noexcept {
    const uint16_t val = getRegisterValue(sensor_registers::SOIL_TEMPERATURE_REG);
    return val != 0xFFFF ? static_cast<float>(val) / 10.0f : -999.0f;
}

uint16_t SoilSensor::readConductivity() noexcept {
    const uint16_t val = getRegisterValue(sensor_registers::SOIL_CONDUCTIVITY_REG);
    return val != 0xFFFF ? static_cast<uint16_t>(val * 10) : 0;
}

float SoilSensor::readPH() noexcept {
    const uint16_t val = getRegisterValue(sensor_registers::SOIL_PH_REG);
    return val != 0xFFFF ? static_cast<float>(val) / 100.0f : -1.0f;
}

uint16_t SoilSensor::readNitrogen() noexcept {
    return getRegisterValue(sensor_registers::SOIL_NITROGEN_REG);
}

uint16_t SoilSensor::readPhosphorus() noexcept {
    return getRegisterValue(sensor_registers::SOIL_PHOSPHORUS_REG);
}

uint16_t SoilSensor::readPotassium() noexcept {
    return getRegisterValue(sensor_registers::SOIL_POTASSIUM_REG);
}

bool SoilSensor::readAll(SensorData &data) noexcept {
    _node.clearResponseBuffer();
    
    uint8_t result = _node.readHoldingRegisters(sensor_registers::SOIL_PH_REG, 1);
    if (result == ModbusMaster::ku8MBSuccess) {
        data.ph = static_cast<float>(_node.getResponseBuffer(0)) / 100.0f;
    } else {
        data.ph = -1.0f;
        return false;
    }

    result = _node.readHoldingRegisters(sensor_registers::SOIL_MOISTURE_REG, 2);
    if (result == ModbusMaster::ku8MBSuccess) {
        data.moisture = static_cast<float>(_node.getResponseBuffer(0)) / 10.0f;
        data.temperature = static_cast<float>(_node.getResponseBuffer(1)) / 10.0f;
    } else {
        data.moisture = -1.0f;
        data.temperature = -999.0f;
        return false;
    }

    result = _node.readHoldingRegisters(sensor_registers::SOIL_CONDUCTIVITY_REG, 1);
    if (result == ModbusMaster::ku8MBSuccess) {
        data.conductivity = _node.getResponseBuffer(0) * 10;
    } else {
        data.conductivity = 0;
        return false;
    }

    result = _node.readHoldingRegisters(sensor_registers::SOIL_NITROGEN_REG, 3);
    if (result == ModbusMaster::ku8MBSuccess) {
        data.nitrogen = _node.getResponseBuffer(0);
        data.phosphorus = _node.getResponseBuffer(1);
        data.potassium = _node.getResponseBuffer(2);
        return true;
    } else {
        data.nitrogen = 0xFFFF;
        data.phosphorus = 0xFFFF;
        data.potassium = 0xFFFF;
    }
    
    return false;
}

bool SoilSensor::setDeviceAddress(uint8_t newAddress) noexcept {
    // JSF AV C++ Rule 90: Do not use magic numbers.
    const uint8_t result = _node.writeSingleRegister(sensor_registers::SOIL_DEVICE_ADDRESS_REG, newAddress);
    if (result == ModbusMaster::ku8MBSuccess && _serial != nullptr) {
        _node.begin(newAddress, *_serial);
        return true;
    }
    return false;
}

bool SoilSensor::setBaudRate(uint16_t baudRateCode) noexcept {
    return _node.writeSingleRegister(sensor_registers::SOIL_BAUD_RATE_REG, baudRateCode) == ModbusMaster::ku8MBSuccess;
}
