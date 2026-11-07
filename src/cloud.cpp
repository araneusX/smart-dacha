#include "cloud.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "secrets.h"
#include "sensors.h"

static const char *kLocationHeader = "Location";

enum class HttpMethod { Post, Get };

static bool sendHttpOnce(HttpMethod method, const String &url, const String &payload, int *codeOut,
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
  return code > 0;
}

static bool postCloudPayload(const String &payload, String *response, bool fetchResult) {
  String url = APPS_SCRIPT_URL;
  url.trim();
  HttpMethod method = HttpMethod::Post;
  String body;
  String location;
  int code = 0;

  for (int hop = 0; hop < 5; hop++) {
    location = "";
    body = "";
    if (!sendHttpOnce(method, url, payload, &code, &body, &location)) {
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

static void fillSensorsJson(JsonArray arr) {
  const Sensors &sensors = getSensors();
  for (uint8_t i = 0; i < sensors.count; i++) {
    if (!sensors.items[i].ok) {
      continue;
    }
    char hex[ROM_HEX_LEN + 1];
    writeSensorRomAsHex(sensors.items[i].rom, hex);
    JsonObject o = arr.add<JsonObject>();
    o["id"] = hex;
    o["t"] = sensors.items[i].t;
  }
}

bool pingCloud(bool *isLive) {
  JsonDocument req;
  req["type"] = 0;
  String payload;
  serializeJson(req, payload);
  String response;
  if (!postCloudPayload(payload, &response, true)) {
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

bool sendCloud(uint8_t type) {
  JsonDocument req;
  req["type"] = type;
  JsonArray sensors = req["sensors"].to<JsonArray>();
  fillSensorsJson(sensors);
  if (sensors.size() == 0) {
    Serial.printf("cloud: type %u skip (no sensors)\n", type);
    return true;
  }
  String payload;
  serializeJson(req, payload);
  if (!postCloudPayload(payload, nullptr, false)) {
    return false;
  }
  Serial.printf("cloud: type %u ok\n", type);
  return true;
}
