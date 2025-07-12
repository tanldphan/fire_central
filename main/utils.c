#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"

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

// CRC error checking algorithm for serial wind sensors
uint16_t cal_CRC(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++)
    {
        crc ^= (uint16_t)data[pos];
        for (uint8_t i =0; i < 8; i++)
        {
            if ((crc & 0x0001) != 0)
            {
                crc = crc >> 1;
                crc ^= 0xA001;
            }
            else crc = crc >> 1;
        }
    }
    return crc;
}

// Convert func: hex array >> byte array
void hex_array_to_byte_array (const char* hex_array, uint8_t* byte_array, const int hex_array_length)
{
    for (int i = 0; i < hex_array_length; i += 2)
    {
        byte_array[i / 2] = hex_char_to_byte (hex_array[i], hex_array[i + 1]);
    }
}

// Convert func: hex char >> byte
uint8_t hex_char_to_byte (char high, char low)
{
    uint8_t byte = 0;
    // Convert high four bits.
    if (high >= '0' && high <= '9')
        byte |= (high - '0') << 4;
    else if (high >= 'A' && high <= 'F')
        byte |= (high - 'A' + 10) << 4;
    else if (high >= 'a' && high <= 'f')
        byte |= (high - 'a' + 10) << 4;

    // Convert low four bits.
    if (low >= '0' && low <= '9')
        byte |= (low - '0');
    else if (low >= 'A' && low <= 'F')
        byte |= (low - 'A' + 10);
    else if (low >= 'a' && low <= 'f')
        byte |= (low - 'a' + 10);

    return byte;
}