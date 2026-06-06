#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "network.h"

#ifndef WIFI_SSID
#define WIFI_SSID "your-ssid"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "your-password"
#endif

#ifndef PC_IP
#define PC_IP "192.168.1.100"
#endif

#ifndef UDP_PORT
#define UDP_PORT 5005
#endif

#ifndef DEVICE_ID
#define DEVICE_ID "footsensor"
#endif

static WiFiUDP udp;
static IPAddress pc_ip;
static uint32_t packet_count = 0;

bool connectWiFi() {
  Serial.printf("# Conectando WiFi a %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("# Error: WiFi no conectado.");
    return false;
  }

  Serial.printf("# WiFi conectado. IP: %s\n", WiFi.localIP().toString().c_str());

  if (!pc_ip.fromString(PC_IP)) {
    Serial.printf("# Error: IP invalida: %s\n", PC_IP);
    return false;
  }

  Serial.printf("# Destino UDP: %s:%d\n", pc_ip.toString().c_str(), UDP_PORT);
  return true;
}

void sendUDP(float ax, float ay, float az, float gx, float gy, float gz, float tempC) {
  packet_count++;

  char buf[512];
  snprintf(buf, sizeof(buf),
    "{"
    "\"device_id\":\"%s\","
    "\"packet\":%lu,"
    "\"timestamp\":%lu,"
    "\"rssi\":%d,"
    "\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,"
    "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,"
    "\"temp_c\":%.2f"
    "}",
    DEVICE_ID,
    (unsigned long)packet_count,
    (unsigned long)millis(),
    (int)WiFi.RSSI(),
    ax, ay, az,
    gx, gy, gz,
    tempC
  );

  udp.beginPacket(pc_ip, UDP_PORT);
  udp.write((const uint8_t*)buf, strlen(buf));
  udp.endPacket();
}
