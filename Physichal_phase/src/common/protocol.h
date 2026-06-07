#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

typedef struct {
  char device_id[16];
  uint32_t packet;
  uint32_t timestamp;
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
  float temp_c;
} SensorPacket;

#endif
