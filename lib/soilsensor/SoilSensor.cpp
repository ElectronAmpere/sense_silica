#include "SoilSensor.h"

SoilSensor* SoilSensor::_instance = nullptr;

SoilSensor::SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin)
    : _node(node), _rePin(rePin), _dePin(dePin) {
    _instance = this;
}

void SoilSensor::begin(Stream &serial, long baud) {
    _serial = &serial;
    _node.begin(1, *_serial); // Default slave ID 1
    pinMode(_rePin, OUTPUT);
    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);
    digitalWrite(_rePin, LOW);
    _node.preTransmission(preTransmission);
    _node.postTransmission(postTransmission);
}

void SoilSensor::preTransmission() {
    if (_instance) {
        digitalWrite(_instance->_rePin, HIGH);
        digitalWrite(_instance->_dePin, HIGH);
        delay(10);
    }
}

void SoilSensor::postTransmission() {
    if (_instance) {
        _instance->_serial->flush(); // Wait for TX buffer to empty
        digitalWrite(_instance->_dePin, LOW);
        digitalWrite(_instance->_rePin, LOW);
    }
}

uint16_t SoilSensor::getRegisterValue(uint16_t reg) {
    #ifdef SOIL_SENSOR_DEBUG
    Serial.print("Reading register: 0x");
    Serial.println(reg, HEX);
    #endif
    uint8_t result = _node.readHoldingRegisters(reg, 1);
    #ifdef SOIL_SENSOR_DEBUG
    Serial.print("Modbus result: ");
    Serial.println(result);
    #endif
    if (result == _node.ku8MBSuccess) {
        uint16_t value = _node.getResponseBuffer(0);
        #ifdef SOIL_SENSOR_DEBUG
        Serial.print("Value: ");
        Serial.println(value);
        #endif
        return value;
    }
    return 0xFFFF; // Error
}

float SoilSensor::readMoisture() {
    uint16_t val = getRegisterValue(SOIL_MOISTURE_REG);
    return val != 0xFFFF ? val / 10.0f : -1.0f;
}

float SoilSensor::readTemperature() {
    uint16_t val = getRegisterValue(SOIL_TEMPERATURE_REG);
    return val != 0xFFFF ? val / 10.0f : -999.0f;
}

uint16_t SoilSensor::readConductivity() {
    return getRegisterValue(SOIL_CONDUCTIVITY_REG);
}

float SoilSensor::readPH() {
    uint16_t val = getRegisterValue(SOIL_PH_REG);
    return val != 0xFFFF ? val / 10.0f : -1.0f;
}

uint16_t SoilSensor::readNitrogen() {
    return getRegisterValue(SOIL_NITROGEN_REG);
}

uint16_t SoilSensor::readPhosphorus() {
    return getRegisterValue(SOIL_PHOSPHORUS_REG);
}

uint16_t SoilSensor::readPotassium() {
    return getRegisterValue(SOIL_POTASSIUM_REG);
}

bool SoilSensor::readAll(SensorData &data) {
    #ifdef SOIL_SENSOR_DEBUG
    Serial.println("Reading all registers...");
    #endif
    uint8_t result = _node.readHoldingRegisters(SOIL_PH_REG, 1);
    if (result == _node.ku8MBSuccess) {
        data.ph = _node.getResponseBuffer(0) / 10.0f;
    } else {
        return false;
    }

    result = _node.readHoldingRegisters(SOIL_MOISTURE_REG, 2);
    if (result == _node.ku8MBSuccess) {
        data.moisture = _node.getResponseBuffer(0) / 10.0f;
        data.temperature = _node.getResponseBuffer(1) / 10.0f;
    } else {
        return false;
    }

    result = _node.readHoldingRegisters(SOIL_CONDUCTIVITY_REG, 1);
    if (result == _node.ku8MBSuccess) {
        data.conductivity = _node.getResponseBuffer(0);
    } else {
        return false;
    }

    result = _node.readHoldingRegisters(SOIL_NITROGEN_REG, 3);
    if (result == _node.ku8MBSuccess) {
        data.nitrogen = _node.getResponseBuffer(0);
        data.phosphorus = _node.getResponseBuffer(1);
        data.potassium = _node.getResponseBuffer(2);
        return true;
    }
    
    #ifdef SOIL_SENSOR_DEBUG
    Serial.println("Failed to read all registers");
    #endif
    return false;
}

bool SoilSensor::setDeviceAddress(uint8_t newAddress) {
    uint8_t result = _node.writeSingleRegister(SOIL_DEVICE_ADDRESS_REG, newAddress);
    if (result == _node.ku8MBSuccess) {
        _node.begin(newAddress, *_serial);
        return true;
    }
    return false;
}

bool SoilSensor::setBaudRate(uint16_t baudRateCode) {
    // 1=2400, 2=4800, 3=9600
    return _node.writeSingleRegister(SOIL_BAUD_RATE_REG, baudRateCode) == _node.ku8MBSuccess;
}
