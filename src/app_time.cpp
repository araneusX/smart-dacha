#include "app_time.h"

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

#include "config.h"

static bool ntpStarted = false;
static bool ntpSynced = false;

static void markNtpSynced(struct timeval *) { ntpSynced = true; }

void startNtp() {
  if (ntpStarted) {
    return;
  }
  sntp_set_time_sync_notification_cb(markNtpSynced);
  configTime(TZ_OFFSET_SEC, 0, NTP_SERVER_1, NTP_SERVER_2);
  ntpStarted = true;
}

int getHistorySlotId() {
  if (!ntpSynced) {
    return 0;
  }
  return static_cast<int>(time(nullptr) / 600);
}
