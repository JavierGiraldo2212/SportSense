#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "common/mpu6500.h"
#include "common/protocol.h"

#ifndef DEVICE_ID
#define DEVICE_ID "footsensor"
#endif

#ifndef RECEIVER_MAC
#error "RECEIVER_MAC must be defined in build_flags. Format: {0xXX,0xXX,0xXX,0xXX,0xXX,0xXX}"
#endif

static uint8_t receiverMac[] = RECEIVER_MAC;
static uint32_t packet_count = 0;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.printf("# ESP-NOW send fail to %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("# TrackSense Sensor Node (ESP-NOW)");

  initMPU6500();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("# Error: ESP-NOW init failed");
    while (true) delay(100);
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("# Error: ESP-NOW add peer failed");
    while (true) delay(100);
  }

  Serial.printf("# ESP-NOW sending to %02X:%02X:%02X:%02X:%02X:%02X\n",
                receiverMac[0], receiverMac[1], receiverMac[2],
                receiverMac[3], receiverMac[4], receiverMac[5]);

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

  SensorPacket pkt;
  strncpy(pkt.device_id, DEVICE_ID, sizeof(pkt.device_id) - 1);
  pkt.device_id[sizeof(pkt.device_id) - 1] = '\0';
  pkt.packet = packet_count++;
  pkt.timestamp = millis();
  pkt.accel_x = ax; pkt.accel_y = ay; pkt.accel_z = az;
  pkt.gyro_x = gx; pkt.gyro_y = gy; pkt.gyro_z = gz;
  pkt.temp_c = tempC;

  esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&pkt, sizeof(pkt));
  if (result != ESP_OK) {
    Serial.println("# ESP-NOW send error");
  }

  Serial.printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f\n",
                ax, ay, az, gx, gy, gz, tempC);

  delay(50);
}
