#include "heaters.h"

#include <Arduino.h>
#include <string.h>

#include "config.h"
#include "sensors.h"

struct Heater {
  int pin;
  uint8_t rom[8];
  bool on;
};

static const int pins[] = {HEATER_RELAY_PIN_1, HEATER_RELAY_PIN_2};
static const char *sensorIds[] = {HEATER_SENSOR_ID_1, HEATER_SENSOR_ID_2};

static constexpr uint8_t kPinSlots = sizeof(pins) / sizeof(pins[0]);
static constexpr uint8_t kIdSlots = sizeof(sensorIds) / sizeof(sensorIds[0]);
static constexpr uint8_t heaterSlots = kPinSlots < kIdSlots ? kPinSlots : kIdSlots;

static Heater heaters[heaterSlots];
static uint8_t heaterCount = 0;

static void setHeaterOutput(Heater *h, bool value) {
  h->on = value;
  digitalWrite(h->pin, value ? HIGH : LOW);
}

void initHeaters() {
  heaterCount = 0;
  for (uint8_t i = 0; i < heaterSlots; i++) {
    if (pins[i] < 0) {
      continue;
    }
    uint8_t rom[8];
    if (!parseSensorRomFromHex(sensorIds[i], rom)) {
      continue;
    }
    Heater *h = &heaters[heaterCount];
    h->pin = pins[i];
    memcpy(h->rom, rom, 8);
    pinMode(h->pin, OUTPUT);
    setHeaterOutput(h, false);
    heaterCount++;
  }
}

void pollHeaters() {
  for (uint8_t i = 0; i < heaterCount; i++) {
    Heater *h = &heaters[i];
    const SensorReading *s = findSensorByRom(h->rom);
    if (!s || !s->ok) {
      setHeaterOutput(h, false);
      continue;
    }
    if (!h->on && s->t < HEATER_ON_BELOW_C) {
      setHeaterOutput(h, true);
    } else if (h->on && s->t > HEATER_OFF_ABOVE_C) {
      setHeaterOutput(h, false);
    }
  }
}
