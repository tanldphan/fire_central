// Wind speed sensor SKU:SEN0483
// Wind direction sensor SKU:SEN0482
// PMS5003
// BME680

// PENDING: wifi mqtt communication with server
// NOTE: don't trust intelliSense

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
#include "rtc.h"
#include "utils.h"
#include "wifi.h"
#include "wind_direction.h"
#include "wind_speed.h"

// Log tags
#define TAG_MONITOR "MONITOR"

// Definitions
static TaskHandle_t monitoring_task;
static TaskHandle_t update_task;
static TaskHandle_t wind_task;

static uint8_t sensor_nodes[MAX_SENSOR_NODES_COUNT][MAC_SIZE] = {0}; // defines dimensions and flushes array.
static uint8_t sensor_nodes_count = 0; // assign node count here.

static char mqtt_packet[200];

// Declaring loop functions for tasks
static void sensor_data_collection();
static void update_sensor_nodes();
static void get_wind_data();
static void send_sensor_data_request_ping(uint8_t* sensor_node);
static void prepare_packet(const union lora_packet_u *lora_sensor_data_packet);
static bool validate_mac(uint8_t* mac, uint8_t* packet, int packet_size);
static void clear_sensor_nodes(void);
//static void print_sensor_nodes(void);


// DUMMY SCHEDULE DURING WAKE TIME
#define SENSOR_NODE_INTERVAL 1000 // ms ----> constant for every batch, calibrate for real use
#define SENSOR_BATCH_INTERVAL 1000 * 60 * 40 //minutes ----> allows multiple batche/attempts during wake time

// Initialize wind data
float wind_speed = 0;
uint8_t wind_direction_indexed = 16;

// BOOT SEQUENCE
void app_main(void)
{
    esp_efuse_mac_get_default(mac_esp); // Fetch ESP's MAC address
    ESP_LOGI(TAG_MONITOR, "Node MAC: %02x%02x%02x%02x%02x%02x\n",
        mac_esp[0], mac_esp[1], mac_esp[2],
        mac_esp[3], mac_esp[4], mac_esp[5]);

    // Initialize functions
    nvs_flash_init(); // ESP's non-volatile storage
    lora_init();
    wifi_init();

    vTaskDelay(pdMS_TO_TICKS(5000));
    mqtt_init();
    rtc_init_me();
    wind_direction_init();
    wind_speed_init();

    vTaskDelay(pdMS_TO_TICKS(5000));

    // Create ESP tasks
    // (functions, name, stack size, arguments, priority, handle reference)
    xTaskCreate(sensor_data_collection, "Monitor", 1024 * 5, NULL, 6, &monitoring_task);
    xTaskCreate(update_sensor_nodes, "Update", 1024 * 5, NULL, 6, &update_task);
    xTaskCreate(get_wind_data, "Central Wind", 1024 *5, NULL, 5, &wind_task);
}

// CORE FUNCTIONS:
static void get_wind_data(void)
{
    while(1)
    {
        float wind_speed = get_wind_direction();
        uint8_t wind_direction_indexed = get_wind_direction();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void send_sensor_data_request_ping(uint8_t *sensor_node)
{
    uint8_t data_ping[MAC_SIZE] = {0};
    // insert mac
    memcpy(&data_ping, sensor_node, MAC_SIZE);
    lora_send_packet(data_ping, sizeof(data_ping));
    ESP_LOGI(pcTaskGetName (NULL), "Data ping sent.");
}

static void prepare_packet(const union lora_packet_u *lora_sensor_data_packet)
{
    sprintf(mqtt_packet, "%02x%02x%02x%02x%02x%02x,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d,%d,%0.2f,%d",
        lora_sensor_data_packet->readings.mac[0], lora_sensor_data_packet->readings.mac[1],
        lora_sensor_data_packet->readings.mac[2], lora_sensor_data_packet->readings.mac[3],
        lora_sensor_data_packet->readings.mac[4], lora_sensor_data_packet->readings.mac[5],
        lora_sensor_data_packet->readings.bme_reading.bme_comp_temp,
        lora_sensor_data_packet->readings.bme_reading.bme_comp_pres,
        lora_sensor_data_packet->readings.bme_reading.bme_comp_humd,
        lora_sensor_data_packet->readings.bme_reading.bme_comp_gas,
        lora_sensor_data_packet->readings.pms_reading.pms_std_1_0,
        lora_sensor_data_packet->readings.pms_reading.pms_std_2_5,
        lora_sensor_data_packet->readings.pms_reading.pms_std_10,
        lora_sensor_data_packet->readings.pms_reading.pms_atm_1_0,
        lora_sensor_data_packet->readings.pms_reading.pms_atm_2_5,
        lora_sensor_data_packet->readings.pms_reading.pms_atm_10,
        wind_speed,
        wind_direction_indexed);
    printf("To be sent: %.*s\n", sizeof(mqtt_packet), mqtt_packet);
}

static void clear_sensor_nodes (void)
{
    for (int i = 0; i < sensor_nodes_count; i++)
    {
        memset(sensor_nodes[i], 0, MAC_SIZE);
    }
}

static bool validate_mac(uint8_t* mac, uint8_t* packet, int packet_size)
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

static void sensor_data_collection()
{
    ESP_LOGI(pcTaskGetName(NULL), "Task Started!"); // Announce current task initiation.
    while (1)
    {
        lora_packet_u sensor_data_packet = {0}; // defines and flushes sensor_data_packet.
        int waited_ms = 0; // initialize time waited.
        if (sensor_nodes_count < 1) 
        {
            ESP_LOGI(pcTaskGetName(NULL), "Waiting for Node Assignment...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else
        {   // Start collecting sequence
            ESP_LOGI(pcTaskGetName(NULL), "%d nodes assigned.", sensor_nodes_count);
            for (int i = 0; i < sensor_nodes_count; i++) // go thru each nodes.
            {
                // send data request ping to node
                send_sensor_data_request_ping(sensor_nodes[i]);
                ESP_LOGI(pcTaskGetName(NULL), "Waiting for node %d (%02x%02x%02x%02x%02x%02x) to respond.", i + 1,
                    sensor_nodes[i][0], sensor_nodes[i][1], sensor_nodes[i][2],
                    sensor_nodes[i][3], sensor_nodes[i][4], sensor_nodes[i][5]); // announce node address currently waiting on.
                // switch to receive mode
                lora_receive();
                while (waited_ms < MAX_WAIT_SECONDS_MS)
                {
                    ESP_LOGD(pcTaskGetName(NULL), "Time passed: %d seconds", waited_ms / 1000);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    // Look for response
                    if (lora_received() == 1)
                    {
                        int packet_length = lora_receive_packet(sensor_data_packet.raw, sizeof(sensor_data_packet.raw));
                        // Validate received packet
                        if ((packet_length == PACKET_SIZE) && (validate_mac(sensor_nodes[i], sensor_data_packet.raw, packet_length) == 1))
                        {
                            ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received.", packet_length);
                            // Process the child sensor data received
                            mqtt_print_packet(sensor_data_packet.raw, packet_length);
                            prepare_packet(&sensor_data_packet);
                            mqtt_publish_reading(mqtt_packet);
                            break;
                        }
                    }
                    waited_ms += 100;
                }
                waited_ms = 0; // reset wait time
                vTaskDelay(pdMS_TO_TICKS(SENSOR_NODE_INTERVAL));
            }
            ESP_LOGI(pcTaskGetName(NULL), "Batch collection done.");
            vTaskDelay(pdMS_TO_TICKS(SENSOR_BATCH_INTERVAL));
        }
    }
}

static void update_sensor_nodes()
{
    ESP_LOGI(pcTaskGetName(NULL), "Task Started!");
    while(1)
    {
        if (sensor_nodes_update_status  == 1)
        {
            ESP_LOGI (pcTaskGetName(NULL), "Nodes List Update Available!");
            // Delete monitoring task if its running so read/write does not happen at
            // the same time
            if (monitoring_task != NULL) // if there's something in monitor log
            {
                vTaskDelete(monitoring_task);
            }
            const int new_sensor_nodes_count = event->data_len % (MAC_SIZE * 2);
            // NOTE: At the current state the function expects a perfectly formated
            // child list come back and see behavior when this isn't the case and
            // ensure safe catching of these exceptions.
            const char* iter = event->data;
            if (new_sensor_nodes_count <= MAX_SENSOR_NODES_COUNT)
            {
                clear_sensor_nodes();
                int nodes_recorded = 0;
                while (nodes_recorded != new_sensor_nodes_count)
                {
                    iter += (MAC_SIZE * 2) + 1;
                    // NOTE: have to be careful that user provides a mac list
                    // with a length divisible by two so that segfault dosen't happen.
                    hex_array_to_byte_array (iter, sensor_nodes[nodes_recorded], MAC_SIZE * 2);
                    ++nodes_recorded;
                }
                ESP_LOGI (TAG_MONITOR, "Child nodes reassigned. Old Count=%d New Count=%d", sensor_nodes_count, new_sensor_nodes_count);
                sensor_nodes_count = new_sensor_nodes_count;
            }
            else
            ESP_LOGW (TAG_MONITOR, "Updated sensor_nodes_count=%d is not <= to MAX_SENSOR_NODES_COUNT=%d", new_sensor_nodes_count, MAX_SENSOR_NODES_COUNT);
            xTaskCreate(sensor_data_collection, "Monitor", 1024 * 5, NULL, 6, &monitoring_task);
            sensor_nodes_update_status = 0;
        }
    }
}