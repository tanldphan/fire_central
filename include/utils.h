#pragma once
#include <stdint.h>
#include <time.h>
#include "mqtt.h"

extern struct tm real_time;

uint16_t cal_CRC(uint8_t *data, uint16_t length);

// Global functions

void hex_array_to_byte_array (const char *hex_array, uint8_t *byte_array, const int hex_array_length);
uint8_t hex_char_to_byte (char high, char low);
void fetch_real_time(void);

// Define PMS data structure
typedef struct
{   // exact 2-byte metrics
    uint16_t pm1_0_std; // Mass concentration Standard particles (ug/m^3)
    uint16_t pm2_5_std; // Particle sizes: <1.0, <2.5, <10 (um)
    uint16_t pm10_std;
    uint16_t pm1_0_atm; // Mass concentration Atmospheric particles ug/m^3
    uint16_t pm2_5_atm;
    uint16_t pm10_atm;
} pms5003_measurement_t;


// Define BME data structure
typedef struct 
{   // 4-byte compensated metrics
    double temp_comp;
    double pres_comp;
    double humd_comp;
    double gas_comp;
} bme680_measurement_t;


// Define LORA packet as type
typedef union lora_packet_u
{
    struct __attribute__ ((packed))
    {
        uint8_t mac[MAC_SIZE];
        pms5003_measurement_t pms_reading;
        bme680_measurement_t bme_reading;
    } reading;
    uint8_t raw[PACKET_SIZE];
} lora_packet_u;