#pragma once

#include <stdint.h>
#include <string.h>

// ESP-IDF included library
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

// LORA assignment
#define LORA_SPI SPI2_HOST // LORA to free SPI channel

#define LORA_SCLK (8)
#define LORA_MISO (18)
#define LORA_MOSI (17)
#define LORA_CS (16)
#define LORA_RST (15)


// LORA model registers:

// 1. Core control:
#define LORA_REG_VERSION 0x42
#define LORA_REG_OP_MODE 0x01
// 2. Frequency:
#define LORA_REG_FRF_MSB 0x06
#define LORA_REG_FRF_MID 0x07
#define LORA_REG_FRF_LSB 0x08
// 3. Power/RF front end:
#define LORA_REG_PA_CONFIG 0x09
#define LORA_REG_LNA 0x0c
// 4. Modem config:
#define LORA_REG_MODEM_CONFIG_1 0x1d
#define LORA_REG_MODEM_CONFIG_2 0x1e
#define LORA_REG_MODEM_CONFIG_3 0x26
#define LORA_REG_PREAMBLE_MSB 0x20
#define LORA_REG_PREAMBLE_LSB 0x21
#define LORA_REG_SYNC_WORD 0x39
// 5. FIFO/buffers:
#define LORA_REG_FIFO 0x00
#define LORA_REG_FIFO_ADDR_PTR 0x0d
#define LORA_REG_FIFO_TX_BASE_ADDR 0x0e
#define LORA_REG_FIFO_RX_BASE_ADDR 0x0f
#define LORA_REG_FIFO_RX_CURRENT_ADDR 0x10
#define LORA_REG_RX_NB_BYTES 0x13
#define LORA_REG_PAYLOAD_LENGTH 0x22
// 6. Interrupts & DIO:
#define LORA_REG_IRQ_FLAGS 0x12
#define LORA_REG_DIO_MAPPING_1 0x40
#define LORA_REG_DIO_MAPPING_2 0x41
// 7. Signal metrics:
#define LORA_REG_PKT_SNR_VALUE 0x19
#define LORA_REG_PKT_RSSI_VALUE 0x1a
#define LORA_REG_RSSI_WIDEBAND 0x2c
// 8. Detection tuning:
#define LORA_REG_DETECTION_OPTIMIZE 0x31
#define LORA_REG_DETECTION_THRESHOLD 0x37

// LORA transceiver modes:
#define LORA_MODE_LONG_RANGE 0x80
#define LORA_MODE_SLEEP 0x00
#define LORA_MODE_STDBY 0x01
#define LORA_MODE_TX 0x03
#define LORA_MODE_RX_CONTINUOUS 0x05
#define LORA_MODE_RX_SINGLE 0x06

// LORA Power Amplifier
#define LORA_PA_BOOST 0x80

// IRQ masks
#define LORA_IRQ_TX_DONE_MASK 0x08
#define LORA_IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define LORA_IRQ_RX_DONE_MASK 0x40
#define LORA_PA_OUTPUT_RFO_PIN 0
#define LORA_PA_OUTPUT_PA_BOOST_PIN 1
#define LORA_TIMEOUT_RESET 100

#define BUFFER_IO 1

// Global LORA functions
int lora_init(void);

void lora_send_packet (uint8_t* buf, int size);
int lora_receive_packet (uint8_t* buf, int size);

void lora_explicit_header_mode (void);
void lora_implicit_header_mode (int size);

void lora_reset (void);
void lora_idle (void);
void lora_sleep (void);
void lora_receive (void);

int lora_get_irq (void);

void lora_set_tx_power (int level);
void lora_set_frequency (long frequency);
void lora_set_spreading_factor (int sf);

int lora_get_spreading_factor (void);
void lora_set_dio_mapping (int dio, int mode);
int lora_get_dio_mapping (int dio);

void lora_set_bandwidth (int sbw);
int lora_get_bandwidth (void);

void lora_set_coding_rate (int cr);
int lora_get_coding_rate (void);

void lora_set_preamble_length (long length);
long lora_get_preamble_length (void);

void lora_set_sync_word (int sw);

void lora_enable_crc (void);
void lora_disable_crc (void);

int lora_received (void);
int lora_packet_lost (void);
int lora_packet_rssi (void);

float lora_packet_snr (void);

void lora_close (void);
int lora_initialized (void);
void lora_dump_registers (void);