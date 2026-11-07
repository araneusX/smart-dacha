#include "sensors.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "telegram.h"

struct OneWireBus {
  int pin;
  OneWire wire;
  DallasTemperature dallas;
};

static const int kSensorPins[] = {SENSOR_PIN_1, SENSOR_PIN_2, SENSOR_PIN_3};
static constexpr uint8_t kSensorPinSlots = sizeof(kSensorPins) / sizeof(kSensorPins[0]);
static OneWireBus buses[kSensorPinSlots];
static uint8_t busCount = 0;
static Sensors sensors;

void writeSensorRomAsHex(const uint8_t rom[8], char out[ROM_HEX_LEN + 1]) {
  static const char hex[] = "0123456789ABCDEF";
  for (int i = 0; i < 8; i++) {
    out[i * 2] = hex[(rom[i] >> 4) & 0x0F];
    out[i * 2 + 1] = hex[rom[i] & 0x0F];
  }
  out[ROM_HEX_LEN] = STR_END;
}

static bool parseHexByte(const char *twoDigits, uint8_t *out) {
  char pair[3] = {twoDigits[0], twoDigits[1], STR_END};
  char *end = nullptr;
  unsigned long value = strtoul(pair, &end, 16);
  if (end != pair + 2) {
    return false;
  }
  *out = static_cast<uint8_t>(value);
  return true;
}

bool parseSensorRomFromHex(const char *hex, uint8_t rom[8]) {
  if (hex == nullptr || strlen(hex) != ROM_HEX_LEN) {
    return false;
  }
  for (int i = 0; i < 8; i++) {
    if (!parseHexByte(hex + i * 2, &rom[i])) {
      return false;
    }
  }
  return true;
}

static bool isRomKnown(const DeviceAddress addr) {
  for (uint8_t i = 0; i < sensors.count; i++) {
    if (memcmp(sensors.items[i].rom, addr, 8) == 0) {
      return true;
    }
  }
  return false;
}

static void addSensor(const DeviceAddress addr, uint8_t bus) {
  SensorReading *s = &sensors.items[sensors.count];
  memcpy(s->rom, addr, 8);
  s->bus = bus;
  s->ok = false;
  s->t = NAN;
  s->status = SENSOR_UNKNOWN;
  sensors.count++;
  char hex[ROM_HEX_LEN + 1];
  writeSensorRomAsHex(s->rom, hex);
  Serial.printf("1-Wire GPIO%d: %s\n", buses[bus].pin, hex);
}

static void scanBus(uint8_t bus) {
  if (sensors.count >= MAX_SENSORS) {
    return;
  }
  DeviceAddress addr;
  buses[bus].wire.reset_search();
  while (sensors.count < MAX_SENSORS && buses[bus].wire.search(addr)) {
    if (OneWire::crc8(addr, 7) != addr[7]) {
      continue;
    }
    if (isRomKnown(addr)) {
      continue;
    }
    addSensor(addr, bus);
  }
}

static void scanBuses() {
  for (uint8_t bus = 0; bus < busCount; bus++) {
    scanBus(bus);
  }
}

static void sendSensorAlert(const SensorReading *s, const String &text) {
  char hex[ROM_HEX_LEN + 1];
  writeSensorRomAsHex(s->rom, hex);
  sendTelegramMessage(String(hex) + text);
}

static void sendSensorAlerts(const SensorReading *s, SensorStatus prev, float prevT) {
  if (prev != SENSOR_UNKNOWN) {
    if (prev == SENSOR_OK && !s->ok) {
      sendSensorAlert(s, ": пропал");
    }
    if (prev == SENSOR_DISCONNECTED && s->ok) {
      sendSensorAlert(s, ": снова на связи");
    }
  }

  const bool wasHigh = prev == SENSOR_OK && prevT > ALERT_TEMP_HIGH_C;
  const bool wasLow = prev == SENSOR_OK && prevT < ALERT_TEMP_LOW_C;
  const bool hot = s->ok && s->t > ALERT_TEMP_HIGH_C;
  const bool cold = s->ok && s->t < ALERT_TEMP_LOW_C;
  if (hot && !wasHigh) {
    sendSensorAlert(s, ": " + String(s->t, 1) + " °C");
  }
  if (cold && !wasLow) {
    sendSensorAlert(s, ": " + String(s->t, 1) + " °C");
  }
  if (s->ok && !hot && !cold && (wasHigh || wasLow)) {
    sendSensorAlert(s, ": температура в норме");
  }
}

void initSensors() {
  busCount = 0;
  for (uint8_t i = 0; i < kSensorPinSlots; i++) {
    if (kSensorPins[i] < 0) {
      continue;
    }
    OneWireBus *bus = &buses[busCount];
    bus->pin = kSensorPins[i];
    bus->wire.begin(static_cast<uint8_t>(bus->pin));
    bus->dallas.setOneWire(&bus->wire);
    bus->dallas.begin();
    bus->dallas.setWaitForConversion(false);
    busCount++;
  }
  sensors.count = 0;
  scanBuses();
}

void pollSensors() {
  if (sensors.count > 0) {
    for (uint8_t bus = 0; bus < busCount; bus++) {
      buses[bus].dallas.requestTemperatures();
    }
    delay(DallasTemperature::millisToWaitForConversion(12));
    for (uint8_t i = 0; i < sensors.count; i++) {
      SensorReading *s = &sensors.items[i];
      const float prevT = s->t;
      const SensorStatus prev = s->status;
      const float t = buses[s->bus].dallas.getTempC(s->rom);
      s->ok = (t > SENSOR_T_MIN_C && t < SENSOR_T_MAX_C);
      s->t = s->ok ? roundf(t * 10.0f) / 10.0f : NAN;
      s->status = s->ok ? SENSOR_OK : SENSOR_DISCONNECTED;
      sendSensorAlerts(s, prev, prevT);
    }
  }
  scanBuses();
}

void printSensorReadings() {
  for (uint8_t i = 0; i < sensors.count; i++) {
    const SensorReading *s = &sensors.items[i];
    char hex[ROM_HEX_LEN + 1];
    writeSensorRomAsHex(s->rom, hex);
    if (s->ok) {
      Serial.printf("%s t=%.1f\n", hex, s->t);
    } else {
      Serial.printf("%s disconnected\n", hex);
    }
  }
}

const Sensors &getSensors() { return sensors; }

const SensorReading *findSensorByRom(const uint8_t rom[8]) {
  if (!rom) {
    return nullptr;
  }
  for (uint8_t i = 0; i < sensors.count; i++) {
    if (memcmp(sensors.items[i].rom, rom, 8) == 0) {
      return &sensors.items[i];
    }
  }
  return nullptr;
}
