//SKU:SEN0483

// ESP-IDF included SDK
#include "esp_task_wdt.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Call headers
#include "wind_speed.h"
#include "utils.h"

// Tag for troubleshooting
#define ERROR_WindSpeed "ERROR"

void wind_speed_init()
{
    uart_config_t UART_cfg =
    { // Default parameters
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,

        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // Not required for RS485
        .source_clk = UART_SCLK_APB, // Use ESP32S3's clock.
    };

    uart_driver_install(WindSpeed_UART, WindSpeed_Buffer_Size * 2, 0, 0, NULL, 0);
    uart_param_config(WindSpeed_UART, &UART_cfg);
    uart_set_pin(WindSpeed_UART, WindSpeed_TX, WindSpeed_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_reset_pin(Direction_CTRL); // Reset directional control pin to default state
    gpio_set_direction(Direction_CTRL, GPIO_MODE_OUTPUT); // set direction control pin as output for RS485
}

float get_wind_speed()
{
    uint8_t RTU_request[8]; // Layout request byte sequence
    uint8_t RTU_response[WindSpeed_Buffer_Size]; // Layout response sequence with buffer size
    
    // Switch to TX mode
    gpio_set_level(Direction_CTRL, 1);

    // Define 8-byte request
    RTU_request[0] = 0x02; // Sensor's default Modbus address
    RTU_request[1] = 0x03; // Function code: to read holding registers
    RTU_request[2] = 0x00; // Starting register address: HIGH BYTE
    RTU_request[3] = 0x00; // Starting register address: LOW BYTE
    RTU_request[4] = 0x00; // Number of registers: HIGH BYTE
    RTU_request[5] = 0x01; // Number of registers: LOW BYTE

    uint16_t crc = cal_CRC(RTU_request, 6); // Calculate CRC
    RTU_request[6] = crc & 0xFF; // Fill 2-byte CRC into last 2 bytes of request
    RTU_request[7] = (crc >> 8) & 0xFF;
    
    // Flush gate in case of backlog
    uart_flush(WindSpeed_UART);
   
    // Send request out
    uart_write_bytes(WindSpeed_UART, RTU_request, sizeof(RTU_request));

    // Switch to RX mode
    gpio_set_level(Direction_CTRL, 0);

    // Read response - max wait 200ms, store number of received bytes
    int received_bytes = uart_read_bytes(WindSpeed_UART, RTU_response, WindSpeed_Buffer_Size, pdMS_TO_TICKS(200));
    
    if (received_bytes < 7)
    {
        ESP_LOGW(ERROR_WindSpeed, "Bad signal -- %d bytes received", received_bytes);
        return -1.0f;
    }
    uint16_t RX_crc = (RTU_response[received_bytes - 1] << 8) | RTU_response[received_bytes - 2]; // Extract response's CRC
    uint16_t calculated_crc = cal_CRC(RTU_response, received_bytes - 2); // Calculate response's CRC
    if (RX_crc != calculated_crc)
        {
            ESP_LOGW(ERROR_WindSpeed, "CRC disparity!");
            return -1.0f;
        }
    return ((RTU_response[3] << 8) | RTU_response[4]) / 10.0; // Return data to function
}