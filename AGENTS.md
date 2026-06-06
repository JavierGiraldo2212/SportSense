# AGENTS.md

## Project structure

```
.
├── Physichal_phase/          # ESP32 firmware (PlatformIO + Arduino framework)
│   ├── platformio.ini         # Board: esp32dev, framework: arduino, monitor: 115200
│   ├── src/main.cpp           # Entrypoint: init MPU6500 → WiFi → calibrate → loop at 20 Hz
│   ├── src/mpu6500.cpp/.h     # MPU6500 IMU driver over I2C (SDA=21, SCL=22, addr=0x68)
│   ├── src/network.cpp/.h     # WiFi station + UDP JSON sender
│   ├── include/ lib/ test/    # PlatformIO stubs (empty)
│   └── .pio/                  # Build artifacts (gitignored)
└── project/
    └── udp_receiver.py        # UDP listener on port 5005, prints JSON payloads
```

## Commands (run from `Physichal_phase/`)

| Action | Command |
|---|---|
| Build | `pio run` |
| Upload | `pio run --target upload` |
| Serial monitor | `pio device monitor` (or `pio run --target monitor`) |
| Clean | `pio run --target clean` |
| Receiver | `python3 ../project/udp_receiver.py` (from project/) |

## Key configuration

All hardware and network config is in **`platformio.ini` `build_flags`** — do NOT edit source files for pins, WiFi credentials, target IP, UDP port, or device ID. Change those defines instead.

| Flag | Default |
|---|---|
| `I2C_SDA_PIN` | 21 |
| `I2C_SCL_PIN` | 22 |
| `MPU_ADDR` | 0x68 |
| `WIFI_SSID` | "Edgar " |
| `WIFI_PASS` | "augusto9" |
| `PC_IP` | "192.168.0.7" |
| `UDP_PORT` | 5005 |
| `DEVICE_ID` | "footsensor" |

Secrets are hardcoded in `platformio.ini`. This is a known risk — do not commit the repo publicly without redacting them.

## Architecture notes

- **Loop rate**: 50 ms delay → ~20 Hz. Each iteration reads MPU6500, converts raw values to physical units (accel: g, gyro: °/s, temp: °C), sends UDP JSON, and prints CSV to serial.
- **Calibration**: 1000 samples at ~3 ms each. Device must be still and flat. Offsets are stored in global `extern` variables in `mpu6500.h`.
- **WiFi**: 20 s timeout (40 attempts × 500 ms). If it fails, the device continues without networking.
- **UDP packet format**: JSON with keys `device_id`, `packet`, `timestamp` (millis), `rssi`, `accel_x/y/z`, `gyro_x/y/z`, `temp_c`.
- **MPU6500 WHO_AM_I**: expected value `0x70`. A mismatch logs a warning but does not block startup.

## No tests, no CI, no lint/format config

This is a minimal IoT prototype. There are no automated tests, CI workflows, linters, or formatters configured.
