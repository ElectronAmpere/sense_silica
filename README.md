# Sense Silica — RS485 Soil 7-in-1 (Modbus RTU)

This project reads an RS485 soil sensor (NPK + temperature, humidity, pH, conductivity) over Modbus RTU using a lightweight real-time scheduler (RIoS).

## Parameters

| **Parameter**                  | **Value**                                     |
|--------------------------------|-----------------------------------------------|
| Power supply                   | 5-24V DC                                      |
| Output signal                  | RS485                                         |
| Temperature range              | -40℃ to 80℃                                  |
| Temperature accuracy           | ±0.5℃                                         |
| Temperature resolution         | 0.1℃                                          |
| Water measurement range        | 0-100%                                       |
| Moisture accuracy              | ±3% (Within 0-53%); ±5% (Within 53-100%)     |
| Water resolution               | 0.10%                                        |
| Electrical conductivity range  | 0-10000 µS/cm                              |
| Conductivity resolution        | 10 µS/cm                                   |
| pH measurement range           | 3-9 pH                                    |
| pH measurement accuracy        | ±0.3 pH                                   |
| pH resolution                  | 0.01 pH                                   |
| NPK measurement range          | 0-1999 mg/kg                               |
| NPK measurement accuracy       | ±2% F.S                                   |
| NPK resolution                 | 1 mg/kg (mg/L)                            |
| Storage environment            | -20℃ to 60℃                               |
| Working pressure range         | 0.9-1.1 atm                               |
| Response time                  | <1s                                       |
| Protection level               | IP68                                       |

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
- `modbus_client_request_raw(cfg, req, reqLen, outBuf, maxOut, outLen)`: Diagnostic raw send/receive.
- `modbus_client_probe_addresses(cfg, addrs[], addrCount, &outAddr)`: Probe for a responding device on the current baud.
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

See `src/main.cpp` for initialization and periodic polling. On startup, the app probes common baud rates (9600/4800/2400) and addresses (1–5); if a device responds, it logs and uses the detected pair. To adjust defaults, change `NPK_RS485_DEFAULT.baudRate` in `include/rs485.h`.

## Notes

- The Modbus client enforces the initial silent interval and validates CRC. Add a post‑response silent interval if polling faster than ~100 ms.
- Temperature is signed: register value is 0.1 °C per unit; negative values are two’s complement.
- RE/DE polarity: `ModbusClientConfig` supports `reActiveLow` and `deActiveHigh` for MAX485 and similar. If wiring is inverted, adjust these flags accordingly.

## LCD Wiring (JHD 16×2, HD44780‑compatible)

This project uses a JHD 16×2 character LCD in 4‑bit mode via the local `lib/lcd` driver. Connect as follows:

- Vss → GND
- Vdd → 5V
- Vee → 10k potentiometer wiper (pot ends to 5V and GND) for contrast
- RS → Arduino D12
- R/W → GND (write‑only)
- E → Arduino D11
- DB0–DB3 → not used (leave unconnected)
- DB4 → Arduino D10
- DB5 → Arduino D9
- DB6 → Arduino D8
- DB7 → Arduino D7
- LED+ → 5V through a 220Ω resistor (if not on module)
- LED− → GND

### Notes

- Common ground: tie Arduino GND, LCD Vss/LED−, and sensor/RS485 grounds together.
- Start the 10k pot mid‑position; adjust Vee until characters are visible.
- Backlight current is typically ~15–20 mA; prefer a series resistor and avoid driving LED+ directly from an I/O pin.
- Only DB4–DB7 are used in 4‑bit mode; ensure R/W is tied to GND.

### Code

- Initialization occurs in `src/main.cpp` with `gLcd.begin()` and periodic updates in `Task_LcdUpdate()`.
- Pin mappings are defined in `include/config.h` under the `pins` namespace.

### LCD Quick Test

To validate wiring independently of the sensor, upload this minimal sketch (pin map matches the project):

```cpp
#include <Arduino.h>
#include "lcd.h"

LCD lcd(12, 11, 5, 4, 3, 2); // RS, E, D4–D7

void setup() {
  lcd.begin();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hello JHD 16x2");
  lcd.setCursor(0, 1);
  lcd.print("Pins RS12 E11");
}

void loop() {}
```

Build and upload with PlatformIO:

```bash
C:\Users\xvj1kor\.platformio\penv\Scripts\platformio.exe run --target upload
```

### Reference

[Arduino with HD44780 based Character LCDs](https://www.martyncurrey.com/arduino-with-hd44780-based-lcds)
