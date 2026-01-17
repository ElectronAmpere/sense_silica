/**
 * @file SoilSensor.cpp
 * @author VigneshRR
 * @brief Implementation of the SoilSensor library.
 * @version 0.2
 * @date 2026-01-17
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "SoilSensor.h"

// Initialize the static instance pointer to null. This is used by the static
// pre/post-transmission callbacks to access the instance's members.
SoilSensor* SoilSensor::_instance = nullptr;

/**
 * @brief See SoilSensor.h for documentation.
 */
SoilSensor::SoilSensor(ModbusMaster &node, uint8_t rePin, uint8_t dePin)
    : _node(node), _rePin(rePin), _dePin(dePin) {
    // Set the static instance pointer to this object. This allows the static
    // callback functions to access the pin numbers for RS485 direction control.
    _instance = this;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
void SoilSensor::begin(Stream &serial, long baud) {
    _serial = &serial;
    // Initialize the ModbusMaster instance with a default slave ID of 1.
    // The actual slave ID of the sensor should be configured to match this,
    // or changed using setDeviceAddress().
    _node.begin(1, *_serial);
    
    // Configure the RS485 direction control pins as outputs.
    pinMode(_rePin, OUTPUT);
    pinMode(_dePin, OUTPUT);
    
    // Set the transceiver to receive mode by default.
    digitalWrite(_dePin, LOW);
    digitalWrite(_rePin, LOW);
    
    // Register the pre- and post-transmission callbacks with the ModbusMaster library.
    // These functions will be called automatically to manage the RS485 transceiver's direction.
    _node.preTransmission(preTransmission);
    _node.postTransmission(postTransmission);
}

/**
 * @brief See SoilSensor.h for documentation.
 */
void SoilSensor::preTransmission() {
    if (_instance) {
        // Set the RS485 transceiver to transmit mode.
        digitalWrite(_instance->_rePin, HIGH);
        digitalWrite(_instance->_dePin, HIGH);
        // A short delay is often required for the transceiver to switch modes.
        delay(10);
    }
}

/**
 * @brief See SoilSensor.h for documentation.
 */
void SoilSensor::postTransmission() {
    if (_instance) {
        // Ensure all data has been sent before switching back to receive mode.
        _instance->_serial->flush();
        // Set the RS485 transceiver back to receive mode.
        digitalWrite(_instance->_dePin, LOW);
        digitalWrite(_instance->_rePin, LOW);
    }
}

/**
 * @brief See SoilSensor.h for documentation.
 */
uint16_t SoilSensor::getRegisterValue(uint16_t reg) {
    // Perform a Modbus read holding registers transaction.
    uint8_t result = _node.readHoldingRegisters(reg, 1);
    
    // If the transaction is successful, return the value from the response buffer.
    if (result == _node.ku8MBSuccess) {
        return _node.getResponseBuffer(0);
    }
    
    // Return an error value if the transaction failed.
    return 0xFFFF;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
float SoilSensor::readMoisture() {
    uint16_t val = getRegisterValue(SOIL_MOISTURE_REG);
    // Scale the raw value according to the sensor's documentation.
    return val != 0xFFFF ? val / 10.0f : -1.0f;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
float SoilSensor::readTemperature() {
    uint16_t val = getRegisterValue(SOIL_TEMPERATURE_REG);
    // Scale the raw value according to the sensor's documentation.
    return val != 0xFFFF ? val / 10.0f : -999.0f;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
uint16_t SoilSensor::readConductivity() {
    uint16_t val = getRegisterValue(SOIL_CONDUCTIVITY_REG);
    return val != 0xFFFF ? val * 10 : 0;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
float SoilSensor::readPH() {
    uint16_t val = getRegisterValue(SOIL_PH_REG);
    // Scale the raw value according to the sensor's documentation.
    return val != 0xFFFF ? val / 100.0f : -1.0f;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
uint16_t SoilSensor::readNitrogen() {
    return getRegisterValue(SOIL_NITROGEN_REG);
}

/**
 * @brief See SoilSensor.h for documentation.
 */
uint16_t SoilSensor::readPhosphorus() {
    return getRegisterValue(SOIL_PHOSPHORUS_REG);
}

/**
 * @brief See SoilSensor.h for documentation.
 */
uint16_t SoilSensor::readPotassium() {
    return getRegisterValue(SOIL_POTASSIUM_REG);
}

/**
 * @brief See SoilSensor.h for documentation.
 */
bool SoilSensor::readAll(SensorData &data) {
    _node.clearResponseBuffer();
    // The sensor's registers are not in a single contiguous block, so we must
    // perform multiple read transactions.
    
    uint8_t result = _node.readHoldingRegisters(SOIL_PH_REG, 1);
    if (result == _node.ku8MBSuccess) {
        data.ph = _node.getResponseBuffer(0) / 100.0f;
    } else {
        data.ph = -1.0f;
        return false;
    }

    result = _node.readHoldingRegisters(SOIL_MOISTURE_REG, 2);
    if (result == _node.ku8MBSuccess) {
        data.moisture = _node.getResponseBuffer(0) / 10.0f;
        data.temperature = _node.getResponseBuffer(1) / 10.0f;
    } else {
        data.moisture = -1.0f;
        data.temperature = -999.0f;
        return false;
    }

    result = _node.readHoldingRegisters(SOIL_CONDUCTIVITY_REG, 1);
    if (result == _node.ku8MBSuccess) {
        data.conductivity = _node.getResponseBuffer(0) * 10;
    } else {
        data.conductivity = 0;
        return false;
    }

    result = _node.readHoldingRegisters(SOIL_NITROGEN_REG, 3);
    if (result == _node.ku8MBSuccess) {
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

/**
 * @brief See SoilSensor.h for documentation.
 */
bool SoilSensor::setDeviceAddress(uint8_t newAddress) {
    // Perform a Modbus write single register transaction to change the device address.
    uint8_t result = _node.writeSingleRegister(SOIL_DEVICE_ADDRESS_REG, newAddress);
    
    // If successful, re-initialize the ModbusMaster instance with the new address.
    if (result == _node.ku8MBSuccess) {
        _node.begin(newAddress, *_serial);
        return true;
    }
    return false;
}

/**
 * @brief See SoilSensor.h for documentation.
 */
bool SoilSensor::setBaudRate(uint16_t baudRateCode) {
    // The sensor accepts specific codes for baud rate changes.
    // 1=2400, 2=4800, 3=9600
    return _node.writeSingleRegister(SOIL_BAUD_RATE_REG, baudRateCode) == _node.ku8MBSuccess;
}
