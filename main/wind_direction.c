#include "esp_task_wdt.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "wind_direction.h"

void wind_direction_init()
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(WIND_DIRECTION_ADC, ADC_ATTEN_DB_11);
}

float get_wind_direction()
{
    return adc1_get_raw(WIND_DIRECTION_ADC) / 4095.0f * 360.0f;
}