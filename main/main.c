// Wind speed sensor SKU:SEN0483
// Wind direction sensor SKU:SEN0482

// C standard library
#include <stdio.h>
#include <string.h>

// ESP-IDF included SDK
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Call headers
#include "wind_speed.h"
#include "wind_direction.h"
#include "utils.h"

#define LOG_TAG "READ"

void app_main(void)
{
    initiate_WindSpeed();
    initiate_WindDirection();
    while(1)
    {
        float wind_speed = Get_WindSpeed();
        uint8_t indexed = Get_WindDirection();
            ESP_LOGI(LOG_TAG, "Wind: %.2f m/s %s", wind_speed, DirectionList[indexed]);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}