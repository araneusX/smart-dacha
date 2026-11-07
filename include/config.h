#pragma once

#define STR_END '\0'

#define MAX_SENSORS 10
#define SENSOR_PIN_1 4
#define SENSOR_PIN_2 16
#define SENSOR_PIN_3 -1
#define SENSOR_T_MIN_C -55.0f
#define SENSOR_T_MAX_C 125.0f

#define SAMPLE_INTERVAL_MS 3000
#define PING_INTERVAL_MS 20000
#define LIVE_INTERVAL_MS 3000
#define LIVE_DELTA_C 0.1f

#define TZ_OFFSET_SEC (3 * 3600)
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"

#define SERIAL_BAUD 115200

#define ALERT_TEMP_HIGH_C 100.0f
#define ALERT_TEMP_LOW_C 0.0f
#define WIFI_RECONNECT_MS 15000

#define HEATER_RELAY_PIN_1 25
#define HEATER_RELAY_PIN_2 26
#define HEATER_SENSOR_ID_1 ""
#define HEATER_SENSOR_ID_2 ""
#define HEATER_ON_BELOW_C 5.0f
#define HEATER_OFF_ABOVE_C 10.0f
