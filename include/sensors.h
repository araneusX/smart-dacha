#pragma once

#include <stdint.h>

#include "config.h"

#define ROM_HEX_LEN 16

enum SensorStatus : uint8_t {
  SENSOR_UNKNOWN = 0,
  SENSOR_OK,
  SENSOR_DISCONNECTED,
};

struct SensorReading {
  uint8_t rom[8];
  uint8_t bus;
  float t;
  bool ok;
  SensorStatus status;
  const char *label;
  const char *id;
};

struct Sensors {
  SensorReading items[MAX_SENSORS];
  uint8_t count;
};

void initSensors();
void pollSensors();
void printSensorReadings();
const Sensors &getSensors();
const SensorReading *findSensorByRom(const uint8_t rom[8]);
bool parseSensorRomFromHex(const char *hex, uint8_t rom[8]);
void writeSensorRomAsHex(const uint8_t rom[8], char out[ROM_HEX_LEN + 1]);
