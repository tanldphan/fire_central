#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

// Call headers
#include "mqtt.h"

// Local MQTT log tag
static const char* TAG_MQTT = "MQTT";

static esp_mqtt_client_handle_t client;
static char mac_address_hex[MAC_SIZE * 2 + 1] = { 0 };
static const char* sensor_nodes_data_topic = "mqtt_test_fire";
static const char* sensor_nodes_assign_topic = "mqtt_test_fire_child";

esp_mqtt_event_handle_t event = NULL;
uint8_t sensor_nodes_update_status = 0;
uint8_t mac_esp[MAC_SIZE] = {0};

void mqtt_init()
{
    sprintf(mac_address_hex, "%02x%02x%02x%02x%02x%02x", mac_esp[0], mac_esp[1], mac_esp[2],
             mac_esp[3], mac_esp[4], mac_esp[5]);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_HOST
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    // Register MQTT event handler.
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, &mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    ESP_LOGI (TAG_MQTT, "MQTT client initialized and started.");
}


void mqtt_event_handler(void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    int msg_id;
    event = event_data;
    esp_mqtt_client_handle_t event_client = event->client;
    switch ((esp_mqtt_event_id_t) event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT Event: CONNECTED");
        msg_id = esp_mqtt_client_subscribe (event_client, sensor_nodes_assign_topic, 0);
        ESP_LOGI(TAG_MQTT, "Sent subscribe successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT Event: DISCONNECTED");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "MQTT Event: PUBLISHED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG_MQTT, "MQTT Event: SUBSCRIBED.");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI (TAG_MQTT, "MQTT Event: DATA RECEIVED");
        // TODO: act accordingly on data event.
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        if (mqtt_validate_topic (event->topic, sensor_nodes_assign_topic)
            && mqtt_validate_central_mac (event->data, mac_address_hex))
        {
            ESP_LOGI (TAG_MQTT, "Received child update event.");
            sensor_nodes_update_status = 1;
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE (TAG_MQTT, "MQTT Event: ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            mqtt_log_error_if_nonzero ("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            mqtt_log_error_if_nonzero ("reported from tls stack", event->error_handle->esp_tls_stack_err);
            mqtt_log_error_if_nonzero ("captured as transport's socket errno",
                                  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI (TAG_MQTT, "Last errno string (%s)",
                      strerror (event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI (TAG_MQTT, "Other event: id=%d", event->event_id);
        break;
    }
}


void mqtt_publish_reading (char* message_to_host)
{
    ESP_LOGI(TAG_MQTT, "Publishing: %s", message_to_host);
    esp_mqtt_client_publish(client, sensor_nodes_data_topic, message_to_host, 0, 2, 0);
}


int mqtt_validate_topic (const char* event_topic, const char* desired_topic)
{
    return strncmp (event_topic, desired_topic, strlen (desired_topic)) == 0;
}


int mqtt_validate_central_mac (const char* child_assignment_central_mac, const char* current_central_mac)
{
    return strncmp (child_assignment_central_mac, current_central_mac, MAC_SIZE * 2) == 0;
}


void mqtt_log_error_if_nonzero (const char* message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE (TAG_MQTT, "Last error %s: 0x%x", message, error_code);
    }
}


void mqtt_print_packet (uint8_t* packet, int packet_size)
{
    for (int i = 0; i < packet_size; i++)
    {
        printf ("%d ", packet[i]);
    }
    printf ("\n");
}
