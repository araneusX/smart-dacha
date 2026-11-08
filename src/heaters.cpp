#include "heaters.h"

#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "sensors.h"
#include "telegram.h"

struct HeaterRelay {
  int pin;
  bool on;
  const char *sensorRomHex;
  const SensorReading *sensor;
  float sensorPrevT;
};

static const int pins[] = {HEATER_RELAY_PIN_1, HEATER_RELAY_PIN_2};
static const char *sensorRoms[] = {HEATER_SENSOR_ROM_1, HEATER_SENSOR_ROM_2};

static constexpr uint8_t heaterSlots = sizeof(pins) / sizeof(pins[0]);
static_assert(sizeof(sensorRoms) / sizeof(sensorRoms[0]) == heaterSlots, "heater rom count");

static HeaterRelay heaters[heaterSlots];
static uint8_t heaterCount = 0;

static void setHeaterRelayOutput(HeaterRelay *h, bool value) {
  if (h->on == value) {
    return;
  }
  h->on = value;
  digitalWrite(h->pin, value ? LOW : HIGH);
  Serial.printf("heater: GPIO%d %s\n", h->pin, value ? "on" : "off");
}

static void bindHeaterSensor(HeaterRelay *h) {
  if (h->sensor) {
    return;
  }
  uint8_t rom[8];
  const bool ok = parseSensorRomFromHex(h->sensorRomHex, rom);
  if (!ok) {
    return;
  }
  h->sensor = findSensorByRom(rom);
}

static void sendHeaterTempAlert(const SensorReading *s, bool heaterOn, const String &text) {
  const String status = heaterOn ? ", обогрев вкл" : ", обогрев выкл";
  if (s->label != nullptr) {
    sendTelegramMessage(String(s->label) + text + status);
    return;
  }
  char hex[ROM_HEX_LEN + 1];
  writeSensorRomAsHex(s->rom, hex);
  sendTelegramMessage(String(hex) + text + status);
}

static void pollHeaterTempAlerts(HeaterRelay *h) {
  const SensorReading *s = h->sensor;
  if (!s || !s->ok) {
    return;
  }

  const bool wasHigh = h->sensorPrevT > ALERT_TEMP_HIGH_C;
  const bool wasLow = h->sensorPrevT < ALERT_TEMP_LOW_C;
  const bool hot = s->t > ALERT_TEMP_HIGH_C;
  const bool cold = s->t < ALERT_TEMP_LOW_C;

  if (hot && !wasHigh) {
    sendHeaterTempAlert(s, h->on, ": Высокая температура: " + String(s->t, 1) + " °C");
  }
  if (cold && !wasLow) {
    sendHeaterTempAlert(s, h->on, ": Низкая температура: " + String(s->t, 1) + " °C");
  }
  if (!hot && !cold && (wasHigh || wasLow)) {
    sendHeaterTempAlert(s, h->on, ": Температура в норме: " + String(s->t, 1) + " °C");
  }

  h->sensorPrevT = s->t;
}

void initHeaters() {
  heaterCount = 0;
  for (uint8_t i = 0; i < heaterSlots; i++) {
    if (pins[i] < 0) {
      continue;
    }
    uint8_t rom[8];
    const bool ok = parseSensorRomFromHex(sensorRoms[i], rom);
    if (!ok) {
      continue;
    }
    HeaterRelay *h = &heaters[heaterCount];
    h->pin = pins[i];
    h->sensorRomHex = sensorRoms[i];
    h->sensor = nullptr;
    h->on = true;
    h->sensorPrevT = NAN;
    pinMode(h->pin, OUTPUT);
    setHeaterRelayOutput(h, false);
    heaterCount++;
  }
}

void pollHeaters() {
  for (uint8_t i = 0; i < heaterCount; i++) {
    HeaterRelay *h = &heaters[i];
    bindHeaterSensor(h);
    const SensorReading *s = h->sensor;
    if (!s || !s->ok) {
      setHeaterRelayOutput(h, false);
    } else if (!h->on && s->t < HEATER_ON_BELOW_C) {
      setHeaterRelayOutput(h, true);
    } else if (h->on && s->t > HEATER_OFF_ABOVE_C) {
      setHeaterRelayOutput(h, false);
    }

    pollHeaterTempAlerts(h);
  }
}
