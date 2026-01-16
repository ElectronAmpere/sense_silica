#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdint.h>
#include "modbus_client.h"
#include "modbus_registers.h"

typedef struct {
    float ph;             // pH (0.01 pH per register unit)
    float moisturePct;    // %RH (0.1 % per register unit)
    float temperatureC;   // °C (0.1 °C per register unit, signed)
    uint16_t conductivity; // µS/cm
    uint16_t nitrogen;     // mg/kg
    uint16_t phosphorus;   // mg/kg
    uint16_t potassium;    // mg/kg
} SoilData;

typedef struct {
    ModbusClientConfig* client;
    uint8_t address; // sensor slave address (default 0x01)
} SoilSensor;

void soil_sensor_init(SoilSensor* s, ModbusClientConfig* client, uint8_t address);

bool soil_sensor_read_ph(SoilSensor* s, float* ph);
bool soil_sensor_read_moisture_temperature(SoilSensor* s, float* moisturePct, float* temperatureC);
bool soil_sensor_read_conductivity(SoilSensor* s, uint16_t* conductivity);
bool soil_sensor_read_npk(SoilSensor* s, uint16_t* n, uint16_t* p, uint16_t* k);
bool soil_sensor_read_all(SoilSensor* s, SoilData* out);

// Configuration registers
bool soil_sensor_set_address(SoilSensor* s, uint8_t newAddress);
bool soil_sensor_set_baud_rate(SoilSensor* s, uint16_t baud);

#endif // SOIL_SENSOR_H