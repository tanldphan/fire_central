// SKU:SEN0482

#pragma once

// ESP-IDF included SDK
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WindDirection_UART UART_NUM_2
#define WindDirection_Buffer_Size (128)
#define WindDirection_TX (6)
#define WindDirection_RX (7)
#define Direction_CTRL (15)

// Declare global functions
void wind_direction_init();
uint8_t get_wind_direction();

// Wind directions definitions:
//extern const char *DirectionList[17];
// to be interpreted on server side