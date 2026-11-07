#pragma once

#include <stdint.h>

#define ROM_HEX_LEN 16

struct SensorReading {
  char id[ROM_HEX_LEN + 1];
  float t;
  bool ok;
};

void sensorsBegin();
uint8_t sensorsCount();
void sensorsRead();
const SensorReading *sensorsGet(uint8_t index);
int sensorsFind(const char *id);
