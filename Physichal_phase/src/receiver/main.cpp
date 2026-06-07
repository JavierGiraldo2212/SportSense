#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "common/protocol.h"

#ifndef DEVICE_ID
#define DEVICE_ID "footsensor-receiver"
#endif

static SensorPacket lastPacket;
static volatile bool newData = false;

void OnDataRecv(const uint8_t *mac, const uint8_t *incoming, int len) {
  if (len == sizeof(SensorPacket)) {
    memcpy(&lastPacket, incoming, sizeof(lastPacket));
    newData = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("# TrackSense Receiver Node (ESP-NOW)");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("# Error: ESP-NOW init failed");
    while (true) delay(100);
  }

  esp_now_register_recv_cb(OnDataRecv);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("# ESP-NOW ready. Receiver MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("# Copy this MAC into [env:sensor] build_flags as RECEIVER_MAC");
  Serial.println("# Use format: -D RECEIVER_MAC='{0xXX,0xXX,0xXX,0xXX,0xXX,0xXX}'");
  Serial.println("# --- JSON output follows ---");
}

void loop() {
  if (newData) {
    newData = false;

    Serial.printf("{\"device_id\":\"%s\",\"packet\":%lu,\"timestamp\":%lu,"
                  "\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,"
                  "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,\"temp_c\":%.2f}\n",
                  lastPacket.device_id,
                  (unsigned long)lastPacket.packet,
                  (unsigned long)lastPacket.timestamp,
                  lastPacket.accel_x, lastPacket.accel_y, lastPacket.accel_z,
                  lastPacket.gyro_x, lastPacket.gyro_y, lastPacket.gyro_z,
                  lastPacket.temp_c);
  }

  delay(10);
}
