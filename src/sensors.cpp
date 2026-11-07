#include "sensors.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <string.h>

#include "config.h"

static OneWire oneWire(ONE_WIRE_PIN);
static DallasTemperature dallas(&oneWire);
static DeviceAddress roms[MAX_SENSORS];
static SensorReading readings[MAX_SENSORS];
static uint8_t count = 0;

static int cmpRom(const DeviceAddress a, const DeviceAddress b) {
  for (int i = 0; i < 8; i++) {
    if (a[i] < b[i]) {
      return -1;
    }
    if (a[i] > b[i]) {
      return 1;
    }
  }
  return 0;
}

static void romToHex(const DeviceAddress addr, char *out) {
  static const char hex[] = "0123456789ABCDEF";
  for (int i = 0; i < 8; i++) {
    out[i * 2] = hex[(addr[i] >> 4) & 0x0F];
    out[i * 2 + 1] = hex[addr[i] & 0x0F];
  }
  out[ROM_HEX_LEN] = '\0';
}

static void sortRoms() {
  for (uint8_t i = 0; i + 1 < count; i++) {
    for (uint8_t j = 0; j + 1 < count - i; j++) {
      if (cmpRom(roms[j], roms[j + 1]) > 0) {
        DeviceAddress tmp;
        memcpy(tmp, roms[j], 8);
        memcpy(roms[j], roms[j + 1], 8);
        memcpy(roms[j + 1], tmp, 8);
      }
    }
  }
}

void sensorsBegin() {
  dallas.begin();
  dallas.setWaitForConversion(true);
  count = dallas.getDeviceCount();
  if (count > MAX_SENSORS) {
    count = MAX_SENSORS;
  }

  uint8_t found = 0;
  DeviceAddress addr;
  oneWire.reset_search();
  while (found < MAX_SENSORS && oneWire.search(addr)) {
    if (OneWire::crc8(addr, 7) != addr[7]) {
      continue;
    }
    memcpy(roms[found], addr, 8);
    found++;
  }
  count = found;
  sortRoms();

  Serial.printf("1-Wire devices: %u\n", count);
  for (uint8_t i = 0; i < count; i++) {
    romToHex(roms[i], readings[i].id);
    readings[i].ok = false;
    readings[i].t = NAN;
    Serial.printf("  %s\n", readings[i].id);
  }
}

uint8_t sensorsCount() { return count; }

void sensorsRead() {
  if (count == 0) {
    return;
  }
  dallas.requestTemperatures();
  for (uint8_t i = 0; i < count; i++) {
    float t = dallas.getTempC(roms[i]);
    readings[i].ok = (t > -55.0f && t < 125.0f);
    readings[i].t = readings[i].ok ? t : NAN;
  }
}

const SensorReading *sensorsGet(uint8_t index) {
  if (index >= count) {
    return nullptr;
  }
  return &readings[index];
}

int sensorsFind(const char *id) {
  if (!id || !id[0]) {
    return -1;
  }
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(readings[i].id, id) == 0) {
      return i;
    }
  }
  return -1;
}
