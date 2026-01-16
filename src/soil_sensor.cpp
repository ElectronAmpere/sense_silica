#include "soil_sensor.h"
#include "logger.h"

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
    if (s->client->trace) LOGD("SOIL", "read pH");
    uint16_t v = 0;
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_PH, 1, &v)) return false;
    if (ph) *ph = to_ph(v);
    if (s->client->trace) { log_kv_u16("SOIL", "raw", "ph", v); }
    return true;
}

bool soil_sensor_read_moisture_temperature(SoilSensor* s, float* moisturePct, float* temperatureC)
{
    if (s->client->trace) LOGD("SOIL", "read moisture+temp");
    uint16_t vals[2] = {0};
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_SOIL_MOISTURE, 2, vals)) return false;
    if (moisturePct) *moisturePct = to_pct(vals[0]);
    if (temperatureC) *temperatureC = to_temp((int16_t)vals[1]);
    if (s->client->trace) {
        log_kv_u16("SOIL", "raw", "moisture", vals[0]);
        log_kv_u16("SOIL", "raw", "temp", vals[1]);
    }
    return true;
}

bool soil_sensor_read_conductivity(SoilSensor* s, uint16_t* conductivity)
{
    if (s->client->trace) LOGD("SOIL", "read conductivity");
    uint16_t v = 0;
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_SOIL_CONDUCTIVITY, 1, &v)) return false;
    if (conductivity) *conductivity = v;
    if (s->client->trace) { log_kv_u16("SOIL", "raw", "ec", v); }
    return true;
}

bool soil_sensor_read_npk(SoilSensor* s, uint16_t* n, uint16_t* p, uint16_t* k)
{
    if (s->client->trace) LOGD("SOIL", "read NPK");
    uint16_t vals[3] = {0};
    if (!modbus_client_read_holding(s->client, s->address, MODBUS_REG_SOIL_NITROGEN, 3, vals)) return false;
    if (n) *n = vals[0];
    if (p) *p = vals[1];
    if (k) *k = vals[2];
    if (s->client->trace) {
        log_kv_u16("SOIL", "raw", "N", vals[0]);
        log_kv_u16("SOIL", "raw", "P", vals[1]);
        log_kv_u16("SOIL", "raw", "K", vals[2]);
    }
    return true;
}

bool soil_sensor_read_all(SoilSensor* s, SoilData* out)
{
    if (!out) return false;
    if (s->client->trace) LOGI("SOIL", "read all");
    float ph = 0.0f, moisture = 0.0f, temp = 0.0f;
    uint16_t cond = 0, n = 0, p = 0, k = 0;

    bool ok1 = soil_sensor_read_ph(s, &ph);
    bool ok2 = soil_sensor_read_moisture_temperature(s, &moisture, &temp);
    bool ok3 = soil_sensor_read_conductivity(s, &cond);
    bool ok4 = soil_sensor_read_npk(s, &n, &p, &k);

    if (!(ok1 && ok2 && ok3 && ok4)) {
        if (s->client->trace) LOGE("SOIL", "read all failed");
        return false;
    }

    out->ph = ph;
    out->moisturePct = moisture;
    out->temperatureC = temp;
    out->conductivity = cond;
    out->nitrogen = n;
    out->phosphorus = p;
    out->potassium = k;
    if (s->client->trace) LOGI("SOIL", "read all ok");
    return true;
}

bool soil_sensor_set_address(SoilSensor* s, uint8_t newAddress)
{
    if (s->client->trace) { LOGI("SOIL", "set address"); log_kv_u8("SOIL", "new", "addr", newAddress); }
    if (!modbus_client_write_single(s->client, s->address, MODBUS_REG_DEVICE_ADDRESS, (uint16_t)newAddress)) {
        if (s->client->trace) LOGE("SOIL", "set address failed");
        return false;
    }
    s->address = newAddress;
    if (s->client->trace) LOGI("SOIL", "set address ok");
    return true;
}

bool soil_sensor_set_baud_rate(SoilSensor* s, uint16_t baud)
{
    // Sensor supports 2400/4800/9600
    if (baud != 2400 && baud != 4800 && baud != 9600) return false;
    bool ok = modbus_client_write_single(s->client, s->address, MODBUS_REG_BAUD_RATE, baud);
    if (s->client->trace) {
        if (ok) { LOGI("SOIL", "set baud ok"); log_kv_u16("SOIL", "baud", "bps", baud); }
        else { LOGE("SOIL", "set baud failed"); }
    }
    return ok;
}