
#pragma once

// ESP-IDF included SDK
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"

#define WIND_DIRECTION_ADC ADC1_CHANNEL_3 // GPIO 4

// Declare global functions
void wind_direction_init();
float get_wind_direction();
