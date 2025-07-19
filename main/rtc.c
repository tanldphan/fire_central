// ESP-IDF SDK
#include "esp_sleep.h"

// Call headers
#include "rtc.h"

// UncleRus' drivers
#include "ds3231.h"
#include "i2cdev.h"

static i2c_dev_t ds3231_handle = {0}; // define and flush

void rtc_ext_init(void)
{
    i2cdev_init();

    ds3231_init_desc(&ds3231_handle, RTC_I2C, RTC_SDA, RTC_SCL);
    ds3231_clear_alarm_flags(&ds3231_handle, DS3231_ALARM_1);
    ds3231_clear_alarm_flags(&ds3231_handle, DS3231_ALARM_2);
    gpio_config(&(gpio_config_t){
        .pin_bit_mask = 1ULL << RTC_SQW,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    });
}

void rtc_set_time(const struct tm *time)
{   
    ds3231_set_time(&ds3231_handle, time);
}

void rtc_set_alarm(const struct tm *time)
{
    ds3231_clear_alarm_flags(&ds3231_handle, DS3231_ALARM_1);
    ds3231_clear_alarm_flags(&ds3231_handle, DS3231_ALARM_2);
    ds3231_set_alarm(&ds3231_handle, DS3231_ALARM_1, time, DS3231_ALARM1_MATCH_SECMIN, NULL, 0);
    ds3231_enable_alarm_ints(&ds3231_handle, DS3231_ALARM_1);
}

void rtc_to_dsleep()
{
    esp_sleep_enable_ext0_wakeup(RTC_SQW, 0); // Wake on LOW
    i2cdev_done(); // clear any semaphores
    esp_deep_sleep_start();
}