#pragma once

#define ONE_WIRE_PIN 4
#define MAX_SENSORS 10

#define SAMPLE_INTERVAL_MS 3000
#define PING_INTERVAL_MS 20000
#define LIVE_INTERVAL_MS 10000
#define LIVE_DELTA_C 0.1f

#define TZ_OFFSET_SEC (3 * 3600)
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"

#define SERIAL_BAUD 115200

#define ALERT_TEMP_C 100.0f
#define WIFI_RECONNECT_MS 15000

#define ENABLE_TELEGRAM 1
#define ENABLE_HEATERS 0

#define HEATER_COUNT 2
#define HEATER_MAX 4
#define HEATER_PIN_1 25
#define HEATER_PIN_2 26
#define HEATER_PIN_3 27
#define HEATER_PIN_4 14
#define HEATER_ON_BELOW_C 5.0f
#define HEATER_OFF_ABOVE_C 10.0f
#define HEATER_SENSOR_ID ""
