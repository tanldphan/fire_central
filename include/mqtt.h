#pragma once

// C standard library
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// ESP-IDF included SDK
#include "esp_event_base.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "mqtt_client.h"

// Definitions
#define MAC_SIZE 6
#define PACKET_SIZE 28 + MAC_SIZE
#define QUALITY_OF_SERVICE 1 // 0: send once, move on | 1: send and confirm, may duplicate | 2: send and confirm, no duplicate
#define MAX_SENSOR_NODES_COUNT 10
#define MAX_WAIT_SECONDS_MS (1000 * 5)
#define START_OF_CHILDREN MAC_SIZE + 1
#define MQTT_HOST "mqtt://mqtt.eclipseprojects.io"

extern esp_mqtt_event_handle_t event;
extern uint8_t sensor_nodes_update_status;
extern uint8_t mac_address[MAC_SIZE];


void mqtt_init (void);
void mqtt_event_handler (void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void publish_reading (char* message_to_host);
int validate_topic (const char* event_topic, const char* desired_topic);
int validate_central_mac (const char* child_assignment_central_mac, const char* current_central_mac);
void log_error_if_nonzero (const char* message, int error_code);
void print_packet (uint8_t* packet, int packet_size);