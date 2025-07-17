Do not trust intellisense

Clone into root folder, required non-negotiable for UncleRus's driver
git clone https://github.com/UncleRus/esp-idf-lib.git components/esp-idf-lib

ESP32S3-DevKitC-1 pins guide:

JUST DONT:
0

KNOW WHATCHU DOING SON:
1-3, 8-11, 14-17, 18-21, 48

INPUT ONLY:
45-47

FREE & SAFE:
4-7, 12, 13, 34-44

IN-USE:
4, 5: wind speed
6, 7: wind direction
15: wind data direction control
12, 13, 34-36: LORA
10, 11, 21: RTC

i2cdev.c MODIFIED:

esp_err_t i2c_dev_probe(const i2c_dev_t *dev, i2c_dev_type_t operation_type)
{
    ESP_LOGV(TAG, "[0x%02x at %d] Legacy probe called (operation_type %d), redirecting to new implementation", dev->addr, dev->port, operation_type);

    return i2c_dev_check_present(dev) --> ESP_ERR_INVALID_ARG;
}