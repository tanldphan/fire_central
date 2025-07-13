#pragma once
#include <stdint.h>'
#include "mqtt.h"

uint16_t cal_CRC(uint8_t *data, uint16_t length);

// Global functions

void hex_array_to_byte_array (const char *hex_array, uint8_t *byte_array, const int hex_array_length);
uint8_t hex_char_to_byte (char high, char low);

// Define PMS data structure
typedef struct
{   // exact 2-byte metrics
    uint16_t pms_std_1_0; // Mass concentration Standard particles (ug/m^3)
    uint16_t pms_std_2_5; // Particle sizes: <1.0, <2.5, <10 (um)
    uint16_t pms_std_10;
    uint16_t pms_atm_1_0; // Mass concentration Atmospheric particles ug/m^3
    uint16_t pms_atm_2_5;
    uint16_t pms_atm_10;
} pms_metric_t;


// Define BME data structure
typedef struct 
{   // 4-byte compensated metrics
    int32_t bme_comp_temp;
    int32_t bme_comp_pres;
    int32_t bme_comp_humd;
    int32_t bme_comp_gas;
} bme_metric_t;


// Define LORA packet as type
typedef union lora_packet_u
{
    struct __attribute__ ((packed))
    {
        uint8_t mac[MAC_SIZE];
        pms_metric_t pms_reading;
        bme_metric_t bme_reading;
    } readings;
    uint8_t raw[PACKET_SIZE];
} lora_packet_u;