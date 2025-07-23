#pragma once

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WIND_SPEED_GPIO (5)

void wind_speed_init(void);
float get_wind_speed(void);