#pragma once

// C Standard
#include <time.h>

// I2C assignment
#define RTC_I2C I2C_NUM_0
#define RTC_SDA (10)
#define RTC_SCL (11)
#define RTC_SQW (12)
#define RTC_PERI_PWR (19)

// Log tags
#define RTC "RTC"

// Global functions
void rtc_ext_init(void);
void rtc_set_time(struct tm *time);
void rtc_get_time(struct tm *time);
void rtc_set_alarm(struct tm *time);
void rtc_to_dsleep(void);