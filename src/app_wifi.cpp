#include "app_wifi.h"

#include <WiFi.h>

#include "app_time.h"
#include "config.h"
#include "secrets.h"
#include "telegram.h"

static bool connected = false;
static unsigned long lastAttempt = 0;

void initWifi() { WiFi.mode(WIFI_STA); }

void pollWifi() {
  if (isWifiConnected()) {
    if (!connected) {
      connected = true;
      Serial.println("Wi-Fi connected");
      sendTelegramMessage("Wi-Fi connected");
    }
    startNtp();
    return;
  }
  if (connected) {
    connected = false;
    Serial.println("Wi-Fi connection lost");
  }
  unsigned long now = millis();
  if (lastAttempt != 0 && now - lastAttempt < WIFI_RECONNECT_MS) {
    return;
  }
  lastAttempt = now;
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool isWifiConnected() { return WiFi.status() == WL_CONNECTED; }
