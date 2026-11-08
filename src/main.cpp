#include <Arduino.h>
#include <esp_task_wdt.h>
#include <math.h>

#include "app_time.h"
#include "app_wifi.h"
#include "cloud.h"
#include "config.h"
#include "heaters.h"
#include "router.h"
#include "sensors.h"

static unsigned long lastSample = 0;
static unsigned long lastPing = 0;
static unsigned long lastLive = 0;
static bool isLive = false;
static int lastHistorySlot = 0;
static float liveSentT[MAX_SENSORS];
static bool liveSentOk[MAX_SENSORS];
static uint8_t liveSentCount = 0;
static bool liveHasSnapshot = false;

static bool hasLiveChanged() {
  const Sensors &sensors = getSensors();
  if (!liveHasSnapshot || liveSentCount != sensors.count) {
    return true;
  }
  for (uint8_t i = 0; i < sensors.count; i++) {
    if (sensors.items[i].ok != liveSentOk[i]) {
      return true;
    }
    if (sensors.items[i].ok && fabsf(sensors.items[i].t - liveSentT[i]) >= LIVE_DELTA_C) {
      return true;
    }
  }
  return false;
}

static void rememberLiveSent() {
  const Sensors &sensors = getSensors();
  liveSentCount = sensors.count;
  for (uint8_t i = 0; i < liveSentCount; i++) {
    liveSentOk[i] = sensors.items[i].ok;
    liveSentT[i] = sensors.items[i].ok ? sensors.items[i].t : 0;
  }
  liveHasSnapshot = true;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println("smart-dch");

  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);

  initHeaters();
  initSensors();
  initRouter();
  initWifi();

  lastSample = millis();
}

void loop() {
  esp_task_wdt_reset();

  pollWifi();
  pollRouter();

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;
    pollSensors();
    printSensorReadings();
    pollHeaters();
    if (isWifiConnected()) {
      bool historySent = false;
      const int slot = getHistorySlotId();
      if (slot > 0) {
        if (lastHistorySlot == 0) {
          lastHistorySlot = slot;
        } else if (slot > lastHistorySlot) {
          const bool ok = sendCloud(1);
          if (ok) {
            lastHistorySlot = slot;
            historySent = true;
            lastLive = now;
            rememberLiveSent();
          }
        }
      }

      if (!isLive) {
        liveHasSnapshot = false;
        lastLive = 0;
      } else if (!historySent && (lastLive == 0 || now - lastLive >= LIVE_INTERVAL_MS) &&
                 hasLiveChanged()) {
        const bool ok = sendCloud(2);
        if (ok) {
          lastLive = now;
          rememberLiveSent();
        }
      }
    }
  }

  if (!isWifiConnected()) {
    return;
  }

  if (lastPing == 0 || now - lastPing >= PING_INTERVAL_MS) {
    lastPing = now;
    bool live = false;
    const bool ok = pingCloud(&live);
    if (ok) {
      reportCloudPingResult(true);
      isLive = live;
      Serial.printf("isLive=%s\n", isLive ? "true" : "false");
    } else {
      reportCloudPingResult(false);
    }
  }
}
