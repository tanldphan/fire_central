
// Call headers
#include "rtc.h"
#include "ds3231.h"
#include "i2cdev.h"

static i2c_dev_t dev; // define device descriptor structure, do not touch, defined in ds3231.c

void rtc_init(void)
{
    i2cdev_init(); // built-in i2cdev initialization
    ds3231_init_desc(&dev, RTC_I2C, RTC_SDA, RTC_SCL); // point ds3231 to esp gpio
}

// Get RTC's clock time and check if read ok
bool rtc_get_time(struct tm *current_rtc_rt) // standard C time structure tm
{
    return ds3231_get_time(&dev, current_rtc_rt) == ESP_OK;
}

// Set RTC's internal clock to fetched real time and check if write ok
bool rtc_set_time(const struct tm *server_rt)
{
    return ds3231_set_time(&dev, server_rt) == ESP_OK;
}

// Set RTC next alarm using RTC's precise Alarm1 -- leave Alarm2 untouched
bool rtc_set_alarm(const struct tm *next_alarm)
{
    return ds3231_set_alarm(&dev, DS3231_ALARM_1, next_alarm, DS3231_ALARM1_MATCH_SECMINHOUR, NULL,0) == ESP_OK;
}