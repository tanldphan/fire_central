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
#define PACKET_SIZE (sizeof(pms5003_measurement_t) + sizeof(bme680_measurement_t) + MAC_SIZE)

#define QUALITY_OF_SERVICE 1 // 0: send once, move on | 1: send and confirm, may duplicate | 2: send and confirm, no duplicate
#define MAX_SENSOR_NODES_COUNT 6 // choose how many sensor nodes assigned to central.
#define MAX_WAIT_SECONDS_MS (5000) // choose how long to wait for sensor node response.
#define START_OF_CHILDREN MAC_SIZE + 1
#define MQTT_HOST "mqtt://10.42.0.1:1883" // Broker local IP

extern const char* sensor_nodes_data_topic = "mqtt_test_fire";
extern const char* sensor_nodes_assign_topic = "mqtt_test_fire_child";
extern esp_mqtt_event_handle_t event;
extern uint8_t sensor_nodes_update_status;
extern uint8_t mac_esp[MAC_SIZE];

// Global MQTT functions:
void mqtt_init (void);
void mqtt_event_handler (void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void mqtt_publish_reading (char* message_to_host);
int mqtt_validate_topic (const char* event_topic, const char* desired_topic);
int mqtt_validate_central_mac (const char* child_assignment_central_mac, const char* current_central_mac);
void mqtt_log_error_if_nonzero (const char* message, int error_code);
void mqtt_print_packet (uint8_t* packet, int packet_size);