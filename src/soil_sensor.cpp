#include "soil_sensor.h"

void soil_sensor_init(SoilSensor* s, ModbusClientConfig* client, uint8_t address)
{
    s->client = client;
    s->address = address;
}

static inline float to_ph(uint16_t raw) { return (float)raw / 100.0f; }
static inline float to_pct(uint16_t raw) { return (float)raw / 10.0f; }
static inline float to_temp(int16_t raw) { return (float)raw / 10.0f; }

bool soil_sensor_read_ph(SoilSensor* s, float* ph)
{
    uint16_t v = 0;
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_PH, 1, &v)) return false;
    if (ph) *ph = to_ph(v);
    return true;
}

bool soil_sensor_read_moisture_temperature(SoilSensor* s, float* moisturePct, float* temperatureC)
{
    uint16_t vals[2] = {0};
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_SOIL_MOISTURE, 2, vals)) return false;
    if (moisturePct) *moisturePct = to_pct(vals[0]);
    if (temperatureC) *temperatureC = to_temp((int16_t)vals[1]);
    return true;
}

bool soil_sensor_read_conductivity(SoilSensor* s, uint16_t* conductivity)
{
    uint16_t v = 0;
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_SOIL_CONDUCTIVITY, 1, &v)) return false;
    if (conductivity) *conductivity = v;
    return true;
}

bool soil_sensor_read_npk(SoilSensor* s, uint16_t* n, uint16_t* p, uint16_t* k)
{
    uint16_t vals[3] = {0};
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_SOIL_NITROGEN, 3, vals)) return false;
    if (n) *n = vals[0];
    if (p) *p = vals[1];
    if (k) *k = vals[2];
    return true;
}

bool soil_sensor_read_all(SoilSensor* s, SoilData* out)
{
    if (!out) return false;
    float ph = 0.0f, moisture = 0.0f, temp = 0.0f;
    uint16_t cond = 0, n = 0, p = 0, k = 0;

    bool ok1 = soil_sensor_read_ph(s, &ph);
    bool ok2 = soil_sensor_read_moisture_temperature(s, &moisture, &temp);
    bool ok3 = soil_sensor_read_conductivity(s, &cond);
    bool ok4 = soil_sensor_read_npk(s, &n, &p, &k);

    if (!(ok1 && ok2 && ok3 && ok4)) return false;

    out->ph = ph;
    out->moisturePct = moisture;
    out->temperatureC = temp;
    out->conductivity = cond;
    out->nitrogen = n;
    out->phosphorus = p;
    out->potassium = k;
    return true;
}

bool soil_sensor_set_address(SoilSensor* s, uint8_t newAddress)
{
    if (!modbus_client_write_single(s->client, s->address, MODBUS_REG_DEVICE_ADDRESS, (uint16_t)newAddress)) {
        return false;
    }
    s->address = newAddress;
    return true;
}

bool soil_sensor_set_baud_rate(SoilSensor* s, uint16_t baud)
{
    // Sensor supports 2400/4800/9600
    if (baud != 2400 && baud != 4800 && baud != 9600) return false;
    return modbus_client_write_single(s->client, s->address, MODBUS_REG_BAUD_RATE, baud);
}