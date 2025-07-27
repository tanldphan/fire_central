#pragma once

// C standard library
#include <stdio.h>
#include <string.h>

// ESP-IDF included library
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// Credentials
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#define MAXIMUM_RETRY 5

// PEAP enabled case
#ifdef CONFIG_PEAP_WIFI // is on
#    define PEAP_WIFI_USERNAME CONFIG_PEAP_WIFI_USERNAME
#    define PEAP_WIFI_IDENTITY CONFIG_PEAP_WIFI_USERNAME
#endif

// Flags
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Global WIFI functions:
void wifi_init ();
void wifi_event_handler (void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);