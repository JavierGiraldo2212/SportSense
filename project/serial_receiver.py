#!/usr/bin/env python3
import sys
import json
import serial
from datetime import datetime

DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD

    print(f"Serial Receiver - {port} @ {baud} baud")
    print("-" * 80)

    with serial.Serial(port, baud, timeout=1) as ser:
        while True:
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue
            if line.startswith("#") or line.startswith("---"):
                print(line)
                continue
            if not line.startswith("{"):
                continue

            try:
                pkt = json.loads(line)
                ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                print(f"[{ts}] Packet #{pkt.get('packet')} | "
                      f"Accel: ({pkt['accel_x']:.3f}, {pkt['accel_y']:.3f}, {pkt['accel_z']:.3f}) g | "
                      f"Gyro: ({pkt['gyro_x']:.2f}, {pkt['gyro_y']:.2f}, {pkt['gyro_z']:.2f}) deg/s | "
                      f"Temp: {pkt['temp_c']:.2f} C")
            except (json.JSONDecodeError, KeyError):
                print(f"[RAW] {line}")

if __name__ == "__main__":
    main()
