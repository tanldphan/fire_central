// SKU:SEN0483

#pragma once

// ESP-IDF included SDK:
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "utils.h"

#define WindSpeed_UART UART_NUM_1 // Set WindSpeed thru ESP's channel UART_1
#define WindSpeed_Buffer_Size (128) // Allocate ESP's RAM to hold sensor reading -- 128 is minimum
#define WindSpeed_TX (4) // Set TX to GPIO 4
#define WindSpeed_RX (5) // Set RX to GPIO 5
#define Direction_CTRL (15) // Set direction control to ESP's internal pin 15

// Declare global functions
void wind_speed_init();
float get_wind_speed();