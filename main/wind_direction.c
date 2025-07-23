#include "esp_task_wdt.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "wind_direction.h"

static adc_oneshot_unit_handle_t adc_handle;

void wind_direction_init()
{
    adc_oneshot_unit_init_cfg_t unit_cfg =
    {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&unit_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, WIND_DIRECTION_ADC, &chan_cfg);
}

float get_wind_direction()
{
    int adc_read; // 0 to 4095 possible analog reads
    adc_oneshot_read(adc_handle, WIND_DIRECTION_ADC, &adc_read);
    return adc_read / 4095.0f * 360.0f;
}