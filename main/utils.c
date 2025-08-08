#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sntp.h"
#include "sys/time.h"
#include "esp_log.h"

#include "utils.h"
#include "rtc.h"
struct tm real_time = {0};

static const struct tm dummy_time = {
    .tm_year = 2025 - 1900,
    .tm_mon  = 7,   // August
    .tm_mday = 1,
    .tm_hour = 9,
    .tm_min  = 0,
    .tm_sec  = 0
};

#define TAG_SNTP "SNTP"

// CRC error checking algorithm for serial wind sensors
// uint16_t cal_CRC(uint8_t *data, uint16_t length)
// {
//     uint16_t crc = 0xFFFF;
//     for (uint16_t pos = 0; pos < length; pos++)
//     {
//         crc ^= (uint16_t)data[pos];
//         for (uint8_t i =0; i < 8; i++)
//         {
//             if ((crc & 0x0001) != 0)
//             {
//                 crc = crc >> 1;
//                 crc ^= 0xA001;
//             }
//             else crc = crc >> 1;
//         }
//     }
//     return crc;
// }

//Convert func: hex array >> byte array
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

void fetch_real_time(void)
{
    const int target_year = 2025;
    const int max_retries = 10;
    int retry = 0;

    esp_sntp_setoperatingmode(0);
    esp_sntp_setservername(0, "time.google.com");
    esp_sntp_init(); // Starts background sync. ESP time will update internally; no direct fetch call needed.

    // filter out garbage reads
    while (real_time.tm_year < (target_year - 1900) && ++retry < max_retries)
    {
        time_t now = time(NULL);
        if (now > 1000) // ignore zero or garbage values
        {
        localtime_r(&now, &real_time); // copy ESP's updated time and paste into real_time
        }    
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // fallback to whatever RTC was able to keep when SNTP fails
    if (real_time.tm_year < 125)
    {
        ESP_LOGW(TAG_SNTP, "failed — falling back to dummy_time.");
        real_time = dummy_time;
        rtc_set_time(&real_time); // set RTC's clock to dummy_time
        settimeofday(&(struct timeval){.tv_sec = mktime(&real_time)}, NULL); // set ESP time to dummy_time
        ESP_LOGI(TAG_SNTP, "RTC fallback time: %02d-%02d-%04d %02d:%02d:%02d",
            real_time.tm_mday, real_time.tm_mon + 1, real_time.tm_year + 1900,
            real_time.tm_hour, real_time.tm_min, real_time.tm_sec);
        esp_sntp_stop();
    }
    else
    {
        ESP_LOGI(TAG_SNTP, "SNTP time acquired. Updating RTC.");
        rtc_set_time(&real_time); // set RTC's clock to real_time
        ESP_LOGI(TAG_SNTP, "SNTP-acquired time: %02d-%02d-%04d %02d:%02d:%02d",
            real_time.tm_mday, real_time.tm_mon + 1, real_time.tm_year + 1900,
            real_time.tm_hour, real_time.tm_min, real_time.tm_sec);
        esp_sntp_stop();
    }
}