#include "esp_task_wdt.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "wind_direction.h"
#include "utils.h"

// Tag for troubleshooting
#define ERROR_WindDirection "ERROR"

// 16 cardinal directions
const char *DirectionList[] =
{
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW",
    "Error!"
};

void wind_direction_init()
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

    uart_driver_install(WindDirection_UART, WindDirection_Buffer_Size * 2, 0, 0, NULL, 0);
    uart_param_config(WindDirection_UART, &UART_cfg);
    uart_set_pin(WindDirection_UART, WindDirection_TX, WindDirection_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_reset_pin(Direction_CTRL); // Reset directional control pin to default state
    gpio_set_direction(Direction_CTRL, GPIO_MODE_OUTPUT); // set direction control pin as output for RS485
}

uint8_t get_wind_direction()
{
    uint8_t RTU_request[8]; // define 8-byte request
    uint8_t RTU_response[WindDirection_Buffer_Size]; // define response with buffer size

    // Switch to TX mode
    gpio_set_level(Direction_CTRL, 1);

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
    uart_flush(WindDirection_UART);

    // Send out request
    uart_write_bytes(WindDirection_UART, RTU_request, sizeof(RTU_request));
    
    // Switch to RX mode
    gpio_set_level(Direction_CTRL, 0);

    // Store response, received bytes -- wait up to 200s
    int received_bytes = uart_read_bytes(WindDirection_UART, RTU_response, sizeof(RTU_response), pdMS_TO_TICKS(200));
    
    if (received_bytes < 7) // Check for signal corruption -- 7 bytes minimum
    {
        ESP_LOGW(ERROR_WindDirection, "Bad signal -- %d bytes received", received_bytes);
        return 16; // Returns error index
    }

    uint16_t RX_crc = (RTU_response[received_bytes - 1] << 8) | RTU_response[received_bytes - 2]; // Extract response's CRC
    uint16_t calculated_crc = cal_CRC(RTU_response, received_bytes - 2); // Calculate response's CRC
    if (RX_crc != calculated_crc) // Check CRC
    {   
        ESP_LOGW(ERROR_WindDirection, "CRC disparity!");
        return 16;
    }

    // Scale, convert sensor's reading to 16 indexed directions
    return (int)(((RTU_response[3] << 8 | RTU_response[4])) / 225) % 16;
}