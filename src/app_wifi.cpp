#include "app_wifi.h"

#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "secrets.h"

static bool wasDisconnected = false;
static bool ntpStarted = false;
static unsigned long lastAttempt = 0;

static void wifiEnsureNtp() {
  if (ntpStarted || WiFi.status() != WL_CONNECTED) {
    return;
  }
  configTime(TZ_OFFSET_SEC, 0, NTP_SERVER_1, NTP_SERVER_2);
  ntpStarted = true;
  Serial.println("NTP started");
}

void wifiBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Wi-Fi connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi IP: ");
    Serial.println(WiFi.localIP());
    wifiEnsureNtp();
  } else {
    Serial.println("Wi-Fi not connected yet");
    wasDisconnected = true;
  }
}

void wifiLoop() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiEnsureNtp();
    return;
  }
  wasDisconnected = true;
  unsigned long now = millis();
  if (now - lastAttempt < WIFI_RECONNECT_MS) {
    return;
  }
  lastAttempt = now;
  Serial.println("Wi-Fi reconnect");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }

bool wifiWasDisconnected() { return wasDisconnected; }

void wifiClearDisconnectFlag() { wasDisconnected = false; }

bool timeIsSynced() {
  struct tm t;
  return getLocalTime(&t, 0);
}

int historySlotId() {
  struct tm t;
  if (!getLocalTime(&t, 0)) {
    return -1;
  }
  return t.tm_year * 366 + t.tm_yday * 144 + t.tm_hour * 6 + t.tm_min / 10;
}

bool historyMinute() {
  struct tm t;
  if (!getLocalTime(&t, 0)) {
    return false;
  }
  return (t.tm_min % 10) == 0;
}
