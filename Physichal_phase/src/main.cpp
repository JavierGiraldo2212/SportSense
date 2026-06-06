#include <Arduino.h>
#include "mpu6500.h"
#include "network.h"

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println("# TrackSense MPU6500 ESP32 UDP");

  initMPU6500();

  if (!connectWiFi()) {
    Serial.println("# Continuando sin WiFi...");
  }

  calibrateMPU(1000);
}

void loop() {
  RawMPU m;

  if (!readRawMPU(m)) {
    Serial.println("# Error leyendo MPU6500");
    delay(200);
    return;
  }

  float ax = (m.ax - ax_offset) / 16384.0f;
  float ay = (m.ay - ay_offset) / 16384.0f;
  float az = (m.az - az_offset) / 16384.0f;

  float gx = (m.gx - gx_offset) / 131.0f;
  float gy = (m.gy - gy_offset) / 131.0f;
  float gz = (m.gz - gz_offset) / 131.0f;

  float tempC = (m.temp / 333.87f) + 21.0f;

  sendUDP(ax, ay, az, gx, gy, gz, tempC);

  Serial.printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f\n",
                ax, ay, az, gx, gy, gz, tempC);

  delay(50);
}
