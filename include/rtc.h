#pragma once

// C Standard
#include <time.h>

// I2C assignment
#define RTC_I2C I2C_NUM_0
#define RTC_SDA (10)
#define RTC_SCL (11)
#define RTC_SQW (21)

// Log tags
#define RTC "RTC"

// Global functions
bool rtc_init(void);
bool rtc_fetch_time(const struct tm *server_rt);
bool rtc_set_alarm(const struct tm *next_alarm);