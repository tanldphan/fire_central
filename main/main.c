// Wind speed sensor SKU:SEN0483
// Wind direction sensor SKU:SEN0482
// PMS5003
// BME680

// PENDING: mqtt communication with server

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
#include "portmacro.h"

// Call headers
#include "wind_speed.h"
#include "wind_direction.h"
#include "utils.h"

// Main
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

// Define PMS data structure
typedef struct
{   // exact 2-byte metrics
    uint16_t pm_std_1_0; // Mass concentration Standard particles (ug/m^3)
    uint16_t pm_std_2_5; // Particle sizes: <1.0, <2.5, <10 (um)
    uint16_t pm_std_10;
    uint16_t pm_atm_1_0; // Mass concentration Atmospheric particles ug/m^3
    uint16_t pm_atm_2_5;
    uint16_t pm_atm_10;
} pms_metric_t;

// Define BME data structure
typedef struct 
{   // 4-byte compensated metrics
    int32_t bme_comp_temp;
    int32_t bme_comp_pres;
    int32_t bme_comp_humd;
    int32_t bme_comp_gas;
} bme_metric_t;

// Define LoRa package
union lora_package_u
{
    struct __attribute__ ((packed))
    {
        uint8_t mac[MAC_SIZE];
        pms_metric_t pms_reading;
        bme_metric_t bme_reading;
    } readings;
    uint8_t raw[PACKET_SIZE];
};
