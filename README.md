# Sense Silica — RS485 Soil 7-in-1 (Modbus RTU)

This project reads an RS485 soil sensor (NPK + temperature, humidity, pH, conductivity) over Modbus RTU using a lightweight real-time scheduler (RIoS).

## Architecture

- **Hardware Layer**: AVR + RS485 transceiver. RE/DE pins control half‑duplex.
- **Protocol Layer**: Modbus RTU helpers (`modbus_rtu.h`) for CRC, framing, and silent interval (≥ 4 character times).
- **Transport Layer**: Modbus client (`modbus_client.h/.cpp`) handles RE/DE, timeouts, retries, and generic Read Holding Registers (0x03).
- **Device Layer**: Soil sensor API (`soil_sensor.h/.cpp`) maps registers, converts units, and exposes typed reads.
- **Application Layer**: RIoS scheduler ticks tasks, e.g. LED toggle and sensor polling output.

### Data Flow (Read Example)

1. Task triggers a read (e.g., moisture + temperature).
2. `soil_sensor_read_moisture_temperature()` calls `modbus_client_read_holding()` with address 0x01, start 0x0012, length 2.
3. Modbus client builds frame, waits ≥4 char times, enables DE/RE, writes, returns to RX.
4. Client reads response header, data, CRC; validates via `modbus_rtu_parse_read_response()`.
5. Device layer converts raw to units (0.1 %RH, 0.1 °C signed) and returns typed values.
6. Application prints values (or can publish/store).

## APIs

- `modbus_client_init(cfg)`: Configure RE/DE pins and defaults.
- `modbus_client_read_holding(cfg, address, regStart, qty, outValues)`: Reads `qty` registers.
- `soil_sensor_read_ph(sensor, &ph)` → float pH (0.01 pH per unit).
- `soil_sensor_read_moisture_temperature(sensor, &pct, &tempC)` → moisture %RH (0.1), temperature °C (0.1, signed).
- `soil_sensor_read_conductivity(sensor, &uS)` → µS/cm.
- `soil_sensor_read_npk(sensor, &N, &P, &K)` → mg/kg.
- `soil_sensor_read_all(sensor, &SoilData)` → aggregate.

## Registers (subset)

- pH: `0x0006`
- Moisture: `0x0012`
- Temperature: `0x0013` (two’s complement for < 0)
- Conductivity: `0x0015`
- Nitrogen/Phosphorus/Potassium: `0x001E/0x001F/0x0020`
- Device Address: `0x0100`
- Baud Rate: `0x0101` (2400/4800/9600)

## Usage

See `src/main.cpp` for initialization and periodic polling. To adjust baud rate, change `NPK_RS485_DEFAULT.baudRate` in `include/rs485.h` and rebuild.

## Notes

- The Modbus client enforces the initial silent interval and validates CRC. Add a post‑response silent interval if polling faster than ~100 ms.
- Temperature is signed: register value is 0.1 °C per unit; negative values are two’s complement.
