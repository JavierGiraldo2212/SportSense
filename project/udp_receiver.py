#!/usr/bin/env python3
import socket
import json
from datetime import datetime

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', 5005))

print("UDP Receiver listening on port 5005...")
print("-" * 80)

try:
    while True:
        data, addr = sock.recvfrom(1024)
        
        try:
            payload = json.loads(data.decode('utf-8'))
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            
            print(f"[{timestamp}] From {addr[0]}:{addr[1]}")
            print(f"  Device: {payload.get('device_id')}")
            print(f"  Packet: {payload.get('packet')}")
            print(f"  ESP32 Timestamp: {payload.get('timestamp')} ms")
            print(f"  WiFi RSSI: {payload.get('rssi')} dBm")
            print(f"  Accel: ({payload.get('accel_x')}, {payload.get('accel_y')}, {payload.get('accel_z')})")
            print()
            
        except json.JSONDecodeError:
            print(f"[ERROR] Invalid JSON from {addr}: {data}")
            
except KeyboardInterrupt:
    print("\nShutdown.")
    sock.close()
