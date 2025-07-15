#pragma once

#include <time.h>
#include <stdbool.h>
#include "ds3231.h"
#include "i2cdev.h"

#define RTC_I2C I2C_NUM_0 // assigning ESP's I2C controller 0
#define RTC_SDA (37)
#define RTC_SCL (38)

// Global functions
void rtc_init_me(void);
bool rtc_get_time(struct tm *time);
bool rtc_set_time(const struct tm *time);
bool rtc_set_alarm(const struct tm *alarm_time);
void rtc_cut_power(); 