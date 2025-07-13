// Wind speed sensor SKU:SEN0483
// Wind direction sensor SKU:SEN0482
// PMS5003
// BME680

// PENDING: wifi mqtt communication with server

// C standard library
#include <stdio.h>
#include <string.h>

// ESP-IDF included SDK
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "driver/i2c.h"
#include "esp_efuse.h"

// Call headers
#include "lora.h"
#include "mqtt.h"
#include "utils.h"
#include "wifi.h"
#include "wind_direction.h"
#include "wind_speed.h"

// Main
#define TAG_MONITOR "MONITOR"

// Definitions
static TaskHandle_t monitoring_task;
static TaskHandle_t update_task;

static uint8_t sensor_nodes[MAX_SENSOR_NODES_COUNT][MAC_SIZE] = { 0 };
static uint8_t sensor_nodes_count = 0;

static char mqtt_packet[200];

// void app_main(void)
// {
//     wind_speed_init();
//     wind_direction_init();
//     while(1)
//     {
//         float wind_speed = get_wind_speed();
//         uint8_t indexed = get_wind_direction();
//         ESP_LOGI(TAG_MONITOR, "Wind: %.2f m/s %s", wind_speed, DirectionList[indexed]);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }


// Preprocessing functions
static void sensor_data_collection ();
static void update_sensor_nodes ();
static void send_sensor_data_request_ping (uint8_t* sensor_node);
static void prepare_packet (const union lora_packet_u* lora_sensor_data_packet);
static bool validate_mac (uint8_t* mac, uint8_t* packet, int packet_size);
static void clear_sensor_nodes (void);
static void print_sensor_nodes (void);

// Boot sequence
void app_main (void)
{
    esp_efuse_mac_get_default(mac_esp); // Fetch ESP's MAC address
    ESP_LOGI(TAG_MONITOR, "Node MAC: %02x%02x%02x%02x%02x%02x\n",
        mac_esp[0], mac_esp[1], mac_esp[2],
        mac_esp[3], mac_esp[4], mac_esp[5]);

    // Initialize functions
    nvs_flash_init(); // ESP's non-volatile storage
    lora_init();
    mqtt_init();
    wifi_init();
    wind_direction_init();
    wind_speed_init();

    vTaskDelay(pdMS_TO_TICKS (5000));

    // Create ESP tasks
    // (functions, name, stack size, arguments, priority, handle reference)
    xTaskCreate(sensor_data_collection, "Monitor", 1024 * 5, NULL, 6, &monitoring_task);
    xTaskCreate(update_sensor_nodes, "Update", 1024 * 5, NULL, 6, &update_task);
}

static void sensor_data_collection ()
{
    ESP_LOGI (pcTaskGetName(NULL), "Task Started!"); // Announce current task initiation
    while (1)
    {
        lora_packet_u sensor_data_packet = {0};
        int waited_ms = 0;
        // Wait till update is complete is available
        if (sensor_nodes_count > 0)
        {
            ESP_LOGI (pcTaskGetName (NULL), "Nodes Assigned! Count=%d", sensor_nodes_count);
            for (int i = 0; i < sensor_nodes_count; i++)
            {
                // Request reading from the child node
                send_sensor_data_request_ping (sensor_nodes[i]);
                ESP_LOGI (pcTaskGetName (NULL), "Waiting for node %d (%02x%02x%02x%02x%02x%02x) data.", i + 1,
                          sensor_nodes[i][0], sensor_nodes[i][1], sensor_nodes[i][2], sensor_nodes[i][3],
                          sensor_nodes[i][4], sensor_nodes[i][5]);
                // Put in receive mode
                lora_receive ();
                while (waited_ms < MAX_WAIT_SECONDS_MS)
                {
                    ESP_LOGD (pcTaskGetName (NULL), "Milliseconds passed: %d", waited_ms);
                    vTaskDelay (pdMS_TO_TICKS (100));
                    // Look for the response
                    if (lora_received ())
                    {
                        int packet_length
                            = lora_receive_packet (sensor_data_packet.raw, sizeof (sensor_data_packet.raw));
                        // Validate received packet
                        if ((packet_length == PACKET_SIZE)
                            && (validate_mac (sensor_nodes[i], sensor_data_packet.raw, packet_length)))
                        {

                            ESP_LOGI (pcTaskGetName (NULL), "%d byte packet received.", packet_length);
                            // Process the child sensor data received
                            print_packet (sensor_data_packet.raw, packet_length);
                            prepare_packet (&sensor_data_packet);
                            publish_reading (mqtt_packet);
                            break;
                        }
                    }
                    waited_ms += 100;
                }
                waited_ms = 0;
                vTaskDelay(pdMS_TO_TICKS(20000));
            }
            ESP_LOGI (pcTaskGetName (NULL), "Finished collecting all child node data.");
        }
        else
        {
            ESP_LOGI (pcTaskGetName (NULL), "Waiting for Assignment...");
            vTaskDelay (pdMS_TO_TICKS (2000));
        }
    }
}

static void update_sensor_nodes ()
{
    ESP_LOGI (pcTaskGetName (NULL), "Task Started!");
    while (true)
    {
        if (sensor_nodes_update_status)
        {
            ESP_LOGI (pcTaskGetName (NULL), "Nodes List Update Available!");
            // Delete monitoring task if its running so read/write does not happen at
            // the same time
            if (monitoring_task != NULL)
            {
                vTaskDelete (monitoring_task);
            }
            const int new_sensor_nodes_count = event->data_len % (MAC_SIZE * 2);
            // NOTE: At the current state the function expects a perfectly formated
            // child list come back and see behavior when this isn't the case and
            // ensure safe catching of these exceptions.
            const char* iter = event->data;
            if (new_sensor_nodes_count <= MAX_SENSOR_NODES_COUNT)
            {
                clear_sensor_nodes ();
                int nodes_recorded = 0;
                while (nodes_recorded != new_sensor_nodes_count)
                {
                    iter += (MAC_SIZE * 2) + 1;
                    // NOTE: have to be careful that user provides a mac list
                    // with a length divisible by two so that segfault dosen't happen.
                    hex_array_to_byte_array (iter, sensor_nodes[nodes_recorded], MAC_SIZE * 2);
                    ++nodes_recorded;
                }
                ESP_LOGI (TAG_MONITOR, "Child nodes reassigned. Old Count=%d New Count=%d", sensor_nodes_count,
                          new_sensor_nodes_count);
                sensor_nodes_count = new_sensor_nodes_count;
            }
            else
                ESP_LOGW (TAG_MONITOR, "Updated sensor_nodes_count=%d is not <= to MAX_SENSOR_NODES_COUNT=%d",
                          new_sensor_nodes_count, MAX_SENSOR_NODES_COUNT);

            xTaskCreate (sensor_data_collection, "Monitor", 1024 * 5, NULL, 6, &monitoring_task);
            sensor_nodes_update_status = 0;
        }
    }
}

static void send_sensor_data_request_ping (uint8_t* sensor_node)
{
    uint8_t data_ping[MAC_SIZE] = { 0 };
    // insert mac
    memcpy (&data_ping, sensor_node, MAC_SIZE);
    lora_send_packet (data_ping, sizeof (data_ping));
    ESP_LOGI (pcTaskGetName (NULL), "Data ping sent.");
}

static void prepare_packet (const union lora_packet_u* lora_sensor_data_packet)
{
    sprintf (mqtt_packet, "%02x%02x%02x%02x%02x%02x,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d,%d",
             lora_sensor_data_packet->reading.mac[0], lora_sensor_data_packet->reading.mac[1],
             lora_sensor_data_packet->reading.mac[2], lora_sensor_data_packet->reading.mac[3],
             lora_sensor_data_packet->reading.mac[4], lora_sensor_data_packet->reading.mac[5],
             lora_sensor_data_packet->reading.bme_reading.temp_comp,
             lora_sensor_data_packet->reading.bme_reading.humd_comp,
             lora_sensor_data_packet->reading.bme_reading.pres_comp,
             lora_sensor_data_packet->reading.bme_reading.gas_comp,
             lora_sensor_data_packet->reading.pms_reading.pm1_0_std,
             lora_sensor_data_packet->reading.pms_reading.pm2_5_std,
             lora_sensor_data_packet->reading.pms_reading.pm10_std,
             lora_sensor_data_packet->reading.pms_reading.pm1_0_atm,
             lora_sensor_data_packet->reading.pms_reading.pm2_5_atm,
             lora_sensor_data_packet->reading.pms_reading.pm10_atm);

    printf ("message to host: %.*s\n", sizeof (mqtt_packet), mqtt_packet);
}

static void clear_sensor_nodes (void)
{
    for (int child = 0; child < sensor_nodes_count; child++)
    {
        memset (sensor_nodes[child], 0, MAC_SIZE);
    }
}

static void print_sensor_nodes (void)
{
    for (int i = 0; i < sensor_nodes_count; i++)
    {
        printf ("Child Node %d: ", i);
        for (int j = 0; j < MAC_SIZE; j++)
        {
            printf ("%02x", sensor_nodes[i][j]);
        }
        printf ("\n");
    }
}

static bool validate_mac (uint8_t* mac, uint8_t* packet, int packet_size)
{
    if (packet_size < MAC_SIZE)
        return 0;
    for (uint8_t i = 0; i < MAC_SIZE; i++)
    {
        if (packet[i] != mac[i])
            return false;
    }
    return true;
}