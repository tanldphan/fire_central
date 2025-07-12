#pragma once
#include <stdint.h>

uint16_t cal_CRC(uint8_t *data, uint16_t length);

// Global functions

void hex_array_to_byte_array (const char *hex_array, uint8_t *byte_array, const int hex_array_length);
uint8_t hex_char_to_byte (char high, char low);