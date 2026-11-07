#include "cloud.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "secrets.h"
#include "sensors.h"

static const char *kLocationHeader = "Location";

enum class HttpMethod { Post, Get };

static bool requestOnce(HttpMethod method, const String &url, const String &payload, int *codeOut,
                        String *bodyOut, String *locationOut) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setTimeout(20000);
  http.setReuse(false);
  if (!http.begin(client, url)) {
    Serial.println("cloud: begin failed");
    return false;
  }
  const char *keys[] = {kLocationHeader};
  http.collectHeaders(keys, 1);
  int code;
  if (method == HttpMethod::Post) {
    http.addHeader("Content-Type", "text/plain; charset=utf-8");
    code = http.POST(payload);
  } else {
    code = http.GET();
  }
  if (locationOut) {
    *locationOut = http.header(kLocationHeader);
  }
  const bool redirected = code >= 300 && code < 400 && http.header(kLocationHeader).length() > 0;
  if (bodyOut && !redirected) {
    *bodyOut = http.getString();
  } else if (bodyOut) {
    *bodyOut = "";
  }
  if (codeOut) {
    *codeOut = code;
  }
  client.stop();
  http.end();
  return true;
}

static bool postJson(const String &payload, String *response, bool fetchResult) {
  String url = APPS_SCRIPT_URL;
  url.trim();
  HttpMethod method = HttpMethod::Post;
  String body;
  String location;
  int code = 0;

  for (int hop = 0; hop < 5; hop++) {
    location = "";
    body = "";
    if (!requestOnce(method, url, payload, &code, &body, &location)) {
      return false;
    }
    if (code >= 300 && code < 400 && location.length() > 0) {
      Serial.printf("cloud: redirect %d\n", code);
      if (!fetchResult && code != 307 && code != 308) {
        return true;
      }
      url = location;
      method = (code == 307 || code == 308) ? HttpMethod::Post : HttpMethod::Get;
      continue;
    }
    if (response) {
      *response = body;
    }
    if (code < 200 || code >= 300) {
      Serial.printf("cloud: HTTP %d %s\n", code, body.substring(0, 160).c_str());
      return false;
    }
    return true;
  }
  Serial.println("cloud: too many redirects");
  return false;
}

static String sensorsJsonArray() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < sensorsCount(); i++) {
    const SensorReading *s = sensorsGet(i);
    if (!s || !s->ok) {
      continue;
    }
    JsonObject o = arr.add<JsonObject>();
    o["id"] = s->id;
    o["t"] = s->t;
  }
  String out;
  serializeJson(arr, out);
  return out;
}

bool cloudPing(bool *isLive) {
  JsonDocument req;
  req["type"] = 0;
  String payload;
  serializeJson(req, payload);
  String response;
  if (!postJson(payload, &response, true)) {
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    Serial.printf("cloud: ping parse error %s\n", response.substring(0, 80).c_str());
    return false;
  }
  if (isLive) {
    *isLive = doc["isLive"] | false;
  }
  return true;
}

bool cloudSend(uint8_t type) {
  String payload = "{\"type\":";
  payload += String(type);
  payload += ",\"sensors\":";
  payload += sensorsJsonArray();
  payload += '}';
  String response;
  if (!postJson(payload, &response, false)) {
    return false;
  }
  Serial.printf("cloud: type %u ok\n", type);
  return true;
}
