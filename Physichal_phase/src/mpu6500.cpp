#include <Arduino.h>
#include <Wire.h>
#include "mpu6500.h"

#ifndef SDA_PIN
#define SDA_PIN 21
#endif

#ifndef SCL_PIN
#define SCL_PIN 22
#endif

#ifndef MPU_ADDR
#define MPU_ADDR 0x68
#endif

float ax_offset = 0.0f;
float ay_offset = 0.0f;
float az_offset = 0.0f;
float gx_offset = 0.0f;
float gy_offset = 0.0f;
float gz_offset = 0.0f;

static bool writeByte(uint8_t addr, uint8_t reg, uint8_t data) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(data);
  return Wire.endTransmission() == 0;
}

static bool readBytes(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *dest) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  uint8_t received = Wire.requestFrom((int)addr, (int)count);
  if (received != count) return false;
  for (uint8_t i = 0; i < count; i++) dest[i] = Wire.read();
  return true;
}

static uint8_t readByte(uint8_t addr, uint8_t reg) {
  uint8_t data = 0xFF;
  if (readBytes(addr, reg, 1, &data)) return data;
  return data;
}

bool readRawMPU(RawMPU &m) {
  uint8_t raw[14];
  if (!readBytes(MPU_ADDR, 0x3B, 14, raw)) return false;

  m.ax   = (int16_t)((raw[0] << 8) | raw[1]);
  m.ay   = (int16_t)((raw[2] << 8) | raw[3]);
  m.az   = (int16_t)((raw[4] << 8) | raw[5]);
  m.temp = (int16_t)((raw[6] << 8) | raw[7]);
  m.gx   = (int16_t)((raw[8] << 8) | raw[9]);
  m.gy   = (int16_t)((raw[10] << 8) | raw[11]);
  m.gz   = (int16_t)((raw[12] << 8) | raw[13]);
  return true;
}

bool initMPU6500() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.printf("# SDA=%d,SCL=%d,ADDR=0x%02X\n", SDA_PIN, SCL_PIN, MPU_ADDR);

  uint8_t who = readByte(MPU_ADDR, 0x75);
  Serial.printf("# WHO_AM_I=0x%02X\n", who);

  bool detected = (who == 0x70);
  if (!detected) {
    Serial.println("# Advertencia: no se detecto un MPU6500 esperado.");
  }

  writeByte(MPU_ADDR, 0x6B, 0x00);
  delay(100);
  writeByte(MPU_ADDR, 0x6B, 0x01);
  writeByte(MPU_ADDR, 0x1A, 0x03);
  writeByte(MPU_ADDR, 0x1B, 0x00);
  writeByte(MPU_ADDR, 0x1C, 0x00);
  delay(50);

  return detected;
}

void calibrateMPU(int samples) {
  long sum_ax = 0, sum_ay = 0, sum_az = 0;
  long sum_gx = 0, sum_gy = 0, sum_gz = 0;
  RawMPU m;
  int valid = 0;

  Serial.println("# Calibrando MPU6500. Dejalo QUIETO y PLANO...");

  for (int i = 0; i < samples; i++) {
    if (readRawMPU(m)) {
      sum_ax += m.ax;
      sum_ay += m.ay;
      sum_az += m.az;
      sum_gx += m.gx;
      sum_gy += m.gy;
      sum_gz += m.gz;
      valid++;
    }
    delay(3);
  }

  if (valid == 0) {
    Serial.println("# Error: no se pudo calibrar.");
    return;
  }

  float mean_ax = (float)sum_ax / valid;
  float mean_ay = (float)sum_ay / valid;
  float mean_az = (float)sum_az / valid;
  float mean_gx = (float)sum_gx / valid;
  float mean_gy = (float)sum_gy / valid;
  float mean_gz = (float)sum_gz / valid;

  ax_offset = mean_ax;
  ay_offset = mean_ay;
  az_offset = mean_az - 16384.0f;
  gx_offset = mean_gx;
  gy_offset = mean_gy;
  gz_offset = mean_gz;

  Serial.println("# Calibracion terminada.");
  Serial.printf("# OFFSETS_ACC_RAW,%.2f,%.2f,%.2f\n", ax_offset, ay_offset, az_offset);
  Serial.printf("# OFFSETS_GYRO_RAW,%.2f,%.2f,%.2f\n", gx_offset, gy_offset, gz_offset);
}
