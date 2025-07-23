
#pragma once

// ESP-IDF included SDK
#include "esp_adc/adc_oneshot.h"

#define WIND_DIRECTION_ADC ADC_CHANNEL_3 // GPIO 4

// Declare global functions
void wind_direction_init();
float get_wind_direction();