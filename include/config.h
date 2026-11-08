#pragma once

#define STR_END '\0'

#define MAX_SENSORS 6
#define SENSOR_PIN_1 4
#define SENSOR_PIN_2 16
#define SENSOR_PIN_3 17
#define SENSOR_T_MIN_C -55.0f
#define SENSOR_T_MAX_C 125.0f

// S("ROM16hex", "label", "id") — empty label/id → fall back to ROM hex
#define SENSOR_LABEL_MAP                      \
  S("28382F49000000B6", "Щит 1", "Panel1")   \
  S("28A13D490000006E", "Щит 2", "Panel2")

#define SAMPLE_INTERVAL_MS 3000
#define PING_INTERVAL_MS 20000
#define LIVE_INTERVAL_MS 3000
#define LIVE_DELTA_C 0.1f

#define TZ_OFFSET_SEC (3 * 3600)
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"

#define SERIAL_BAUD 115200
#define WATCHDOG_TIMEOUT_SEC 120

#define WIFI_RECONNECT_MS 15000
#define APPS_SCRIPT_FAILS_BEFORE_ALERT 3

#define HEATER_RELAY_PIN_1 25
#define HEATER_RELAY_PIN_2 26
#define HEATER_SENSOR_ROM_1 "28382F49000000B6"
#define HEATER_SENSOR_ROM_2 "28A13D490000006E"
#define HEATER_ON_BELOW_C 5.0f
#define HEATER_OFF_ABOVE_C 10.0f
#define ALERT_TEMP_HIGH_C 50.0f
#define ALERT_TEMP_LOW_C 0.0f

#define ROUTER_RELAY_PIN 27
#define ROUTER_OFFLINE_BEFORE_REBOOT_MS (10UL * 60UL * 1000UL)
#define ROUTER_POWER_OFF_MS 10000
#define ROUTER_TCP_HOST "1.1.1.1"
#define ROUTER_TCP_PORT 53
#define ROUTER_TCP_TIMEOUT_MS 3000
#define ROUTER_HTTPS_URL "https://www.google.com/generate_204"
#define ROUTER_HTTPS_TIMEOUT_MS 8000

#define RESERVE_RELAY_PIN 33
