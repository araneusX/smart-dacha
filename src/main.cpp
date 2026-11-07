#include <Arduino.h>
#include <math.h>

#include "app_wifi.h"
#include "cloud.h"
#include "config.h"
#include "heaters.h"
#include "sensors.h"
#include "telegram.h"

static unsigned long lastSample = 0;
static unsigned long lastPing = 0;
static unsigned long lastLive = 0;
static bool isLive = false;
static int lastHistorySlot = -1;
static float liveSentT[MAX_SENSORS];
static bool liveSentOk[MAX_SENSORS];
static uint8_t liveSentCount = 0;
static bool liveHasSnapshot = false;

static bool liveNeedsSend() {
  if (!liveHasSnapshot || liveSentCount != sensorsCount()) {
    return true;
  }
  for (uint8_t i = 0; i < sensorsCount(); i++) {
    const SensorReading *s = sensorsGet(i);
    if (!s || s->ok != liveSentOk[i]) {
      return true;
    }
    if (s->ok && fabsf(s->t - liveSentT[i]) >= LIVE_DELTA_C) {
      return true;
    }
  }
  return false;
}

static void liveRemember() {
  liveSentCount = sensorsCount();
  for (uint8_t i = 0; i < liveSentCount; i++) {
    const SensorReading *s = sensorsGet(i);
    liveSentOk[i] = s && s->ok;
    liveSentT[i] = (s && s->ok) ? s->t : 0;
  }
  liveHasSnapshot = true;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println("smart-dch");

  heatersBegin();
  sensorsBegin();
  wifiBegin();
  telegramBegin();

  lastSample = millis();
  lastPing = 0;
  lastLive = 0;
  lastHistorySlot = -1;
}

void loop() {
  wifiLoop();
  if (!wifiIsConnected()) {
    heatersFailsafeOff();
  }

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;
    sensorsRead();
    for (uint8_t i = 0; i < sensorsCount(); i++) {
      const SensorReading *s = sensorsGet(i);
      if (!s) {
        continue;
      }
      if (s->ok) {
        Serial.printf("%s t=%.2f\n", s->id, s->t);
      } else {
        Serial.printf("%s disconnected\n", s->id);
      }
    }
    telegramLoop();
    if (wifiIsConnected()) {
      heatersLoop();
    }
  }

  if (!wifiIsConnected()) {
    return;
  }

  if (lastPing == 0 || now - lastPing >= PING_INTERVAL_MS) {
    lastPing = now;
    bool live = false;
    if (cloudPing(&live)) {
      isLive = live;
      Serial.printf("isLive=%s\n", isLive ? "true" : "false");
    }
  }

  if (!isLive) {
    liveHasSnapshot = false;
    lastLive = 0;
  } else if ((lastLive == 0 || now - lastLive >= LIVE_INTERVAL_MS) && liveNeedsSend()) {
    if (cloudSend(2)) {
      lastLive = now;
      liveRemember();
    }
  }

  const int slot = historySlotId();
  if (slot >= 0 && slot != lastHistorySlot && historyMinute()) {
    if (cloudSend(1)) {
      lastHistorySlot = slot;
    }
  }
}
