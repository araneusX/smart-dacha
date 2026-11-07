#include "telegram.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "app_wifi.h"
#include "config.h"
#include "secrets.h"

static constexpr bool kTelegramEnabled =
    TELEGRAM_BOT_TOKEN[0] != STR_END && TELEGRAM_CHAT_ID[0] != STR_END;

bool sendTelegramMessage(const String &text) {
  if (!kTelegramEnabled || !isWifiConnected()) {
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
