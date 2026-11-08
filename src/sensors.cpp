#include "sensors.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "telegram.h"

struct SensorLabelEntry {
  const char *romHex;
  const char *label;
  const char *id;
};

static const SensorLabelEntry kLabelMap[] = {
#define S(romHex, label, id) {romHex, label, id},
    SENSOR_LABEL_MAP
#undef S
};
static constexpr size_t kLabelMapCount = sizeof(kLabelMap) / sizeof(kLabelMap[0]);

static const SensorLabelEntry *findLabelEntry(const uint8_t rom[8]) {
  for (size_t i = 0; i < kLabelMapCount; i++) {
    uint8_t parsed[8];
    const bool ok = parseSensorRomFromHex(kLabelMap[i].romHex, parsed);
    if (!ok) {
      continue;
    }
    if (memcmp(parsed, rom, 8) == 0) {
      return &kLabelMap[i];
    }
  }
  return nullptr;
}

static const char *nonEmptyOrNull(const char *s) {
  if (s == nullptr || s[0] == STR_END) {
    return nullptr;
  }
  return s;
}

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
    const bool ok = parseHexByte(hex + i * 2, &rom[i]);
    if (!ok) {
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
  const SensorLabelEntry *entry = findLabelEntry(s->rom);
  s->label = entry ? nonEmptyOrNull(entry->label) : nullptr;
  s->id = entry ? nonEmptyOrNull(entry->id) : nullptr;
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
  uint8_t attempts = 0;
  while (sensors.count < MAX_SENSORS && attempts < 32) {
    const bool ok = buses[bus].wire.search(addr);
    if (!ok) {
      break;
    }
    attempts++;
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
  if (s->label != nullptr) {
    sendTelegramMessage(String(s->label) + text);
    return;
  }
  char hex[ROM_HEX_LEN + 1];
  writeSensorRomAsHex(s->rom, hex);
  sendTelegramMessage(String(hex) + text);
}

static void sendSensorAlerts(const SensorReading *s, SensorStatus prev) {
  if (prev == SENSOR_UNKNOWN) {
    return;
  }
  if (prev == SENSOR_OK && !s->ok) {
    sendSensorAlert(s, ": связь потеряна");
  }
  if (prev == SENSOR_DISCONNECTED && s->ok) {
    sendSensorAlert(s, ": снова на связи");
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
      const SensorStatus prev = s->status;
      const float t = buses[s->bus].dallas.getTempC(s->rom);
      s->ok = (t > SENSOR_T_MIN_C && t < SENSOR_T_MAX_C);
      s->t = s->ok ? roundf(t * 10.0f) / 10.0f : NAN;
      s->status = s->ok ? SENSOR_OK : SENSOR_DISCONNECTED;
      sendSensorAlerts(s, prev);
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
