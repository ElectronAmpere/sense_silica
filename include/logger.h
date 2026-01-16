#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Simple Serial-based logger for low-resource tracing
// Levels: E, W, I, D, T

static inline void log_line(const char* level, const char* tag, const char* msg) {
    Serial.print('['); Serial.print(level); Serial.print(']');
    Serial.print('['); Serial.print(tag); Serial.print(']');
    Serial.print(' '); Serial.println(msg);
}

static inline void log_kv_u16(const char* level, const char* tag, const char* key, uint16_t val) {
    Serial.print('['); Serial.print(level); Serial.print(']');
    Serial.print('['); Serial.print(tag); Serial.print(']');
    Serial.print(' '); Serial.print(key); Serial.print('='); Serial.println(val);
}

static inline void log_kv_u8(const char* level, const char* tag, const char* key, uint8_t val) {
    Serial.print('['); Serial.print(level); Serial.print(']');
    Serial.print('['); Serial.print(tag); Serial.print(']');
    Serial.print(' '); Serial.print(key); Serial.print('='); Serial.println(val);
}

static inline void log_kv_hex8(const char* level, const char* tag, const char* key, uint8_t val) {
    Serial.print('['); Serial.print(level); Serial.print(']');
    Serial.print('['); Serial.print(tag); Serial.print(']');
    Serial.print(' '); Serial.print(key); Serial.print("=0x"); Serial.println(val, HEX);
}

static inline void log_kv_hex16(const char* level, const char* tag, const char* key, uint16_t val) {
    Serial.print('['); Serial.print(level); Serial.print(']');
    Serial.print('['); Serial.print(tag); Serial.print(']');
    Serial.print(' '); Serial.print(key); Serial.print("=0x"); Serial.println(val, HEX);
}

#define LOGE(tag,msg) log_line("E", tag, msg)
#define LOGW(tag,msg) log_line("W", tag, msg)
#define LOGI(tag,msg) log_line("I", tag, msg)
#define LOGD(tag,msg) log_line("D", tag, msg)
#define LOGT(tag,msg) log_line("T", tag, msg)

#endif // LOGGER_H