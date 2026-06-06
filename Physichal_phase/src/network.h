#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

bool connectWiFi();
void sendUDP(float ax, float ay, float az, float gx, float gy, float gz, float tempC);

#endif
