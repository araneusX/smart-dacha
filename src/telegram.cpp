#include "telegram.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "app_wifi.h"
#include "config.h"
#include "secrets.h"
#include "sensors.h"

#if ENABLE_TELEGRAM

static bool present[MAX_SENSORS];
static bool overheat[MAX_SENSORS];
static bool inited = false;

static bool tokenOk() {
  return TELEGRAM_BOT_TOKEN[0] != '\0' && TELEGRAM_CHAT_ID[0] != '\0';
}

static bool sendMessage(const String &text) {
  if (!tokenOk() || !wifiIsConnected()) {
    return false;
  }
  String url = "https://api.telegram.org/bot";
  url += TELEGRAM_BOT_TOKEN;
  url += "/sendMessage";

  JsonDocument doc;
  doc["chat_id"] = TELEGRAM_CHAT_ID;
  doc["text"] = text;

  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  if (!http.begin(client, url)) {
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  http.end();
  return code >= 200 && code < 300;
}

void telegramBegin() {
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    present[i] = true;
    overheat[i] = false;
  }
  inited = false;
}

void telegramLoop() {
  if (!tokenOk()) {
    return;
  }

  if (wifiWasDisconnected() && wifiIsConnected()) {
    sendMessage("ESP32: Wi-Fi восстановлен");
    wifiClearDisconnectFlag();
  }

  uint8_t n = sensorsCount();
  for (uint8_t i = 0; i < n; i++) {
    const SensorReading *s = sensorsGet(i);
    if (!s) {
      continue;
    }
    bool ok = s->ok;
    if (inited && present[i] && !ok) {
      sendMessage(String(s->id) + ": пропал");
    }
    if (inited && !present[i] && ok) {
      sendMessage(String(s->id) + ": снова на связи");
    }
    present[i] = ok;

    bool hot = ok && s->t > ALERT_TEMP_C;
    if (hot && !overheat[i]) {
      sendMessage(String(s->id) + ": " + String(s->t, 1) + " °C");
    }
    if (!hot && overheat[i]) {
      sendMessage(String(s->id) + ": температура в норме");
    }
    overheat[i] = hot;
  }
  inited = true;
}

#else

void telegramBegin() {}
void telegramLoop() {}

#endif
