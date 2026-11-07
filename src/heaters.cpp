#include "heaters.h"

#include <Arduino.h>

#include "config.h"
#include "sensors.h"

#if ENABLE_HEATERS

static const uint8_t pins[HEATER_MAX] = {
    HEATER_PIN_1, HEATER_PIN_2, HEATER_PIN_3, HEATER_PIN_4};

static bool on[HEATER_MAX];

static void setHeater(uint8_t i, bool value) {
  on[i] = value;
  digitalWrite(pins[i], value ? HIGH : LOW);
}

void heatersBegin() {
  uint8_t n = HEATER_COUNT;
  if (n > HEATER_MAX) {
    n = HEATER_MAX;
  }
  for (uint8_t i = 0; i < n; i++) {
    pinMode(pins[i], OUTPUT);
    setHeater(i, false);
  }
}

void heatersFailsafeOff() { heatersBegin(); }

void heatersLoop() {
  int idx = sensorsFind(HEATER_SENSOR_ID);
  const SensorReading *s = idx >= 0 ? sensorsGet(static_cast<uint8_t>(idx)) : nullptr;
  if (!s || !s->ok) {
    heatersFailsafeOff();
    return;
  }

  uint8_t n = HEATER_COUNT;
  if (n > HEATER_MAX) {
    n = HEATER_MAX;
  }
  for (uint8_t i = 0; i < n; i++) {
    if (!on[i] && s->t < HEATER_ON_BELOW_C) {
      setHeater(i, true);
    } else if (on[i] && s->t > HEATER_OFF_ABOVE_C) {
      setHeater(i, false);
    }
  }
}

#else

void heatersBegin() {}
void heatersLoop() {}
void heatersFailsafeOff() {}

#endif
