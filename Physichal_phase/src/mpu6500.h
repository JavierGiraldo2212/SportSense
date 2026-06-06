#ifndef MPU6500_H
#define MPU6500_H

#include <Arduino.h>

struct RawMPU {
  int16_t ax, ay, az;
  int16_t temp;
  int16_t gx, gy, gz;
};

extern float ax_offset;
extern float ay_offset;
extern float az_offset;
extern float gx_offset;
extern float gy_offset;
extern float gz_offset;

bool initMPU6500();
bool readRawMPU(RawMPU &m);
void calibrateMPU(int samples = 1000);

#endif
