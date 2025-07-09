#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// CRC error checking algorithm
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