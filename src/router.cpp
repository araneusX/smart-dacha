#include "router.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "app_wifi.h"
#include "config.h"
#include "telegram.h"

#if ROUTER_RELAY_PIN >= 0

static void setRouterPower(bool on) { digitalWrite(ROUTER_RELAY_PIN, on ? LOW : HIGH); }

static bool internetOk = false;
static bool probeSeen = false;
static bool appsScriptAlertSent = false;

static unsigned long offlineSince = 0;
static bool rebootAttempted = false;
static unsigned long powerOffUntil = 0;

static bool probeTcp() {
  WiFiClient client;
  client.setTimeout(ROUTER_TCP_TIMEOUT_MS);
  IPAddress ip;
  if (!ip.fromString(ROUTER_TCP_HOST)) {
    return false;
  }
  const bool ok = client.connect(ip, ROUTER_TCP_PORT, ROUTER_TCP_TIMEOUT_MS);
  client.stop();
  return ok;
}

static bool probeHttps() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.setTimeout(ROUTER_HTTPS_TIMEOUT_MS);
  http.setReuse(false);
  if (!http.begin(client, ROUTER_HTTPS_URL)) {
    return false;
  }
  const int code = http.GET();
  client.stop();
  http.end();
  return code > 0 && code < 400;
}

static bool probeInternetFallbacks() {
  if (probeTcp()) {
    Serial.println("router: TCP probe ok");
    return true;
  }
  if (probeHttps()) {
    Serial.println("router: HTTPS probe ok");
    return true;
  }
  return false;
}

void reportCloudPingResult(bool cloudOk) {
  probeSeen = true;

  if (!isWifiConnected()) {
    internetOk = false;
    return;
  }

  if (cloudOk) {
    internetOk = true;
    appsScriptAlertSent = false;
    return;
  }

  const bool fallbackOk = probeInternetFallbacks();
  internetOk = fallbackOk;
  if (fallbackOk && !appsScriptAlertSent) {
    sendTelegramMessage("Apps Script недоступен");
    Serial.println("router: Apps Script down, internet ok");
    appsScriptAlertSent = true;
  }
}

static bool isInternetConnected() {
  if (!isWifiConnected()) {
    return false;
  }
  if (!probeSeen) {
    return true;
  }
  return internetOk;
}

void initRouter() {
  pinMode(ROUTER_RELAY_PIN, OUTPUT);
  setRouterPower(true);
}

void pollRouter() {
  const unsigned long now = millis();

  if (powerOffUntil != 0) {
    if (now >= powerOffUntil) {
      setRouterPower(true);
      powerOffUntil = 0;
      Serial.println("router: power restored after reboot pulse");
    }
    return;
  }

  if (isInternetConnected()) {
    if (offlineSince != 0 && rebootAttempted) {
      const unsigned long offlineSec = (now - offlineSince) / 1000UL;
      String msg = "Роутер был перезагружен, время не в сети: ";
      msg += String(offlineSec);
      msg += " секунд";
      sendTelegramMessage(msg);
      Serial.println(msg);
    }
    offlineSince = 0;
    rebootAttempted = false;
    return;
  }

  if (offlineSince == 0) {
    offlineSince = now;
    Serial.println("router: internet lost");
  }

  if (!rebootAttempted && now - offlineSince >= ROUTER_OFFLINE_BEFORE_REBOOT_MS) {
    Serial.println("router: power off for reboot");
    setRouterPower(false);
    powerOffUntil = now + ROUTER_POWER_OFF_MS;
    rebootAttempted = true;
  }
}

#else

void initRouter() {}
void pollRouter() {}
void reportCloudPingResult(bool) {}

#endif
