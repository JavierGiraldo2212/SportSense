# AGENTS.md

## Project structure

```
.
├── Physichal_phase/              # ESP32 firmware (PlatformIO + Arduino framework)
│   ├── platformio.ini             # Two envs: sensor (default) and receiver
│   ├── src/
│   │   ├── common/
│   │   │   ├── protocol.h           # ESP-NOW SensorPacket struct (shared)
│   │   │   ├── mpu6500.cpp/.h       # MPU6500 IMU driver over I2C (SDA=21, SCL=22, addr=0x68)
│   │   ├── sensor/
│   │   │   └── main.cpp             # Sensor entrypoint: MPU6500 → ESP-NOW send
│   │   └── receiver/
│   │       └── main.cpp             # Receiver entrypoint: ESP-NOW recv → Serial JSON
│   ├── include/ lib/ test/          # PlatformIO stubs (empty)
│   └── .pio/                        # Build artifacts (gitignored)
└── project/
    ├── udp_receiver.py              # Legacy UDP listener on port 5005
    ├── serial_receiver.py           # Reads sensor JSON from USB serial
    └── visualizer.py                # Live 2D/3D plots of IMU data
```

## Commands

Run from `Physichal_phase/` unless noted. Use `-e name` to pick an environment.

| Action | Command |
|---|---|
| Build sensor | `pio run -e sensor` |
| Build receiver | `pio run -e receiver` |
| Upload sensor | `pio run -e sensor --target upload` |
| Upload receiver | `pio run -e receiver --target upload` |
| Serial monitor (sensor) | `pio device monitor -e sensor` |
| Serial monitor (receiver) | `pio device monitor -e receiver` |
| Clean all | `pio run --target clean` |
| Serial receiver (PC) | `python3 ../project/serial_receiver.py [port]` (from project/) |
| Live visualizer (PC) | `python3 ../project/visualizer.py [port]` (from project/) |
| Legacy UDP receiver | `python3 ../project/udp_receiver.py` (from project/) |

`build_src_filter` controls which source files each env compiles — the sensor compiles `common/` + `sensor/`, the receiver only `receiver/`. No env includes both `main.cpp` files.

## Key configuration

All hardware and config is in **`platformio.ini` `build_flags`** — do NOT edit source files. Change defines instead.

| Env | Flag | Default |
|---|---|---|
| both | `DEVICE_ID` | `"footsensor"` / `"footsensor-receiver"` |
| sensor | `I2C_SDA_PIN` | 21 |
| sensor | `I2C_SCL_PIN` | 22 |
| sensor | `MPU_ADDR` | 0x68 |
| sensor | `RECEIVER_MAC` | `{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}` |

**`RECEIVER_MAC`** must be set before uploading the sensor. Flash the receiver first, read its printed MAC from serial, then update `RECEIVER_MAC` in `[env:sensor]` `build_flags`.

## Architecture notes

- **Setup flow** (sensor): init MPU6500 → init WiFi STA + ESP-NOW → add receiver as peer → calibrate → loop at 20 Hz
- **Setup flow** (receiver): init Serial → init WiFi STA + ESP-NOW → register recv callback → print own MAC → loop waits for data
- **Loop rate**: 50 ms delay → ~20 Hz. Each iteration reads MPU6500, converts to physical units (accel: g, gyro: °/s, temp: °C), sends ESP-NOW `SensorPacket`, prints CSV to serial.
- **ESP-NOW packet**: struct `SensorPacket` in `protocol.h` — contains `device_id`, `packet`, `timestamp`, `accel_x/y/z`, `gyro_x/y/z`, `temp_c`.
- **PC receives via Serial**: Receiver prints JSON lines to Serial. Run `serial_receiver.py` to parse and display.
- **Calibration**: 1000 samples at ~3 ms each. Device must be still and flat. Offsets stored in global `extern` in `mpu6500.h`.
- **Receiver MAC**: printed once on boot. Copy-paste into `RECEIVER_MAC` build flag using format `{0xXX,0xXX,0xXX,0xXX,0xXX,0xXX}`.
- **MPU6500 WHO_AM_I**: expected `0x70`. Mismatch logs a warning but does not block startup.

## No tests, no CI, no lint/format config

Minimal IoT prototype. No automated tests, CI workflows, linters, or formatters.
