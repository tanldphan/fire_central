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
#include "esp_sleep.h"

// Call headers
#include "lora.h"
#include "mqtt.h"
#include "rtc.h"
#include "utils.h"
#include "wifi.h"
#include "wind_direction.h"
#include "wind_speed.h"

// Log tags
#define TAG_MAIN "MAIN.C"

// Definitions
static TaskHandle_t monitoring_task;
static TaskHandle_t update_task;
static TaskHandle_t wind_task;

static uint8_t sensor_nodes[MAX_SENSOR_NODES_COUNT][MAC_SIZE] = {0}; // size and flush
static uint8_t sensor_nodes_count = 1; // assign node count here.

static char mqtt_packet[200];

// Declaring loop functions for tasks
static void sensor_data_collection();
static void update_sensor_nodes();
static void get_wind_data();
static void send_sensor_data_request_ping(uint8_t* sensor_node);
static void prepare_packet(const union lora_packet_u *lora_sensor_data_packet);
static bool validate_mac(uint8_t* mac, uint8_t* packet, int packet_size);
static void clear_sensor_nodes(void);
//static void fallback_dsleep(void *nothing);

// Batch and run-thru intervals
#define SENSOR_NODE_INTERVAL 1000 // ms ----> constant for every batch, calibrate for real use
#define SENSOR_BATCH_INTERVAL 1000 * 5 //seconds

// Initialize wind data
float wind_speed = 0;
float wind_direction = 0;

// Dummy server real time
struct tm server_rt = {
    .tm_year = 2025 - 1900,
    .tm_mon  = 1,
    .tm_mday = 1,
    .tm_hour = 1,
    .tm_min  = 1,
    .tm_sec  = 0
};

// BOOT SEQUENCE
void app_main(void)
{
    rtc_ext_init(); // must initialize RTC before wifi or INT will hold LOW
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED)
    {
        ESP_LOGW("RTC WARNING", "COLD BOOT -- SETTING CLOCK");
        rtc_set_time(&server_rt);
    }

    // Fallback alarm
    struct tm fallback_alarm;
    rtc_get_time(&fallback_alarm);
    fallback_alarm.tm_sec += 60;
    mktime(&fallback_alarm);
    rtc_set_alarm(&fallback_alarm);

    //xTaskCreate(fallback_dsleep, "Fallback Deep Sleep", 1024 * 5, NULL, 6, NULL);

    esp_efuse_mac_get_default(mac_esp); // Fetch ESP's MAC address
    ESP_LOGI(TAG_MAIN, "Node MAC: %02x%02x%02x%02x%02x%02x\n",
        mac_esp[0], mac_esp[1], mac_esp[2],
        mac_esp[3], mac_esp[4], mac_esp[5]);

    // TEST SENSOR -- HARDCODED
    uint8_t sensor_1[MAC_SIZE] = {0x7c, 0xdf, 0xa1, 0xe5, 0xc6, 0x74};
    memcpy(sensor_nodes[0], sensor_1, MAC_SIZE);

    // Initialize functions
    nvs_flash_init(); // ESP's non-volatile storage, almost instant
    lora_init(); // max ~250ms delay
    wifi_init(); // ~5-6s delay, depends on # of retries
    mqtt_init(); // does not halt, async, runs in background
    wind_direction_init(); // instant
    wind_speed_init(); // instant

    // Important: sensor node related tasks will block whatever comes after it -- at least when they fail (to be tested).
    // get_wind_data does not block
    // ESP tasks (functions, name, stack size, arguments, priority, handle reference)
    xTaskCreate(sensor_data_collection, "Monitor", 1024 * 5, NULL, 6, &monitoring_task);
    xTaskCreate(update_sensor_nodes, "Update", 1024 * 5, NULL, 6, &update_task);
    xTaskCreate(get_wind_data, "Central Wind", 1024 *5, NULL, 5, &wind_task);
    // TO BE INTEGRATED: Task to override fallback_alarm with server_alarm
    // TO BE INTEGRATED: Write server_rt into RTC's clock
}

// CORE FUNCTIONS:

// static void fallback_dsleep(void *nothing)
// {
//     vTaskDelay(pdMS_TO_TICKS(1000 *30)); // Dummy wake duration
//     // Sleep duration = +alarm - dummy wake
//     if (monitoring_task) vTaskDelete(monitoring_task);
//     if (update_task)     vTaskDelete(update_task);
//     if (wind_task)       vTaskDelete(wind_task);

//     rtc_to_dsleep();
// }

static void get_wind_data(void)
{
    while(1)
    {
        wind_speed = get_wind_speed();
        wind_direction = get_wind_direction();
        ESP_LOGD("WIND", "Wind speed: %f m/s @ deg: %f", wind_speed, wind_direction);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void send_sensor_data_request_ping(uint8_t *sensor_node)
{
    uint8_t data_ping[MAC_SIZE] = {0};
    // insert mac
    memcpy(&data_ping, sensor_node, MAC_SIZE);
    ESP_LOG_BUFFER_HEX("CENTRAL", data_ping, sizeof(data_ping));
    ESP_LOGI("CENTRAL", "Sending packet size: %d", sizeof(data_ping));
    lora_send_packet(data_ping, sizeof(data_ping));
    ESP_LOGI(pcTaskGetName (NULL), "Data ping sent.");
}


// KNOWN ISSUE: Right now, all readings default to 0 even when there's no data.
static void prepare_packet(const union lora_packet_u *lora_sensor_data_packet)
{
    sprintf(
        mqtt_packet,
        "MAC: %02x%02x%02x%02x%02x%02x | "
        "PMS: %u, %u, %u, %u, %u, %u | "
        "BME: %.2f, %.2f, %.2f, %.2f |"
        "WIND: %.2f, %.2f",

        lora_sensor_data_packet->reading.mac[0], lora_sensor_data_packet->reading.mac[1],
        lora_sensor_data_packet->reading.mac[2], lora_sensor_data_packet->reading.mac[3],
        lora_sensor_data_packet->reading.mac[4], lora_sensor_data_packet->reading.mac[5],

        lora_sensor_data_packet->reading.pms_reading.pm1_0_std,
        lora_sensor_data_packet->reading.pms_reading.pm2_5_std,
        lora_sensor_data_packet->reading.pms_reading.pm10_std,
        lora_sensor_data_packet->reading.pms_reading.pm1_0_atm,
        lora_sensor_data_packet->reading.pms_reading.pm2_5_atm,
        lora_sensor_data_packet->reading.pms_reading.pm10_atm,

        lora_sensor_data_packet->reading.bme_reading.temp_comp,
        lora_sensor_data_packet->reading.bme_reading.pres_comp,
        lora_sensor_data_packet->reading.bme_reading.humd_comp,
        lora_sensor_data_packet->reading.bme_reading.gas_comp,

        wind_speed,
        wind_direction
    );
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


// KNOWN ISSUES:
// sensor_data_collection logic is flawed and is blocking MQTT communication on failed packets.
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
            char hex_dump[PACKET_SIZE * 2 + 1] = {0};
            for (int i = 0; i < PACKET_SIZE; ++i)
            {
                sprintf(&hex_dump[i * 2], "%02x", sensor_data_packet.raw[i]);
            }
            ESP_LOGW("PACKET", "Sending packet (hex): %s", hex_dump);
            prepare_packet(&sensor_data_packet);
            mqtt_publish_reading(mqtt_packet); // forcing trash thru MQTT for testing
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
                ESP_LOGI (TAG_MAIN, "Child nodes reassigned. Old Count=%d New Count=%d", sensor_nodes_count, new_sensor_nodes_count);
                sensor_nodes_count = new_sensor_nodes_count;
            }
            else
            ESP_LOGW (TAG_MAIN, "Updated sensor_nodes_count=%d is not <= to MAX_SENSOR_NODES_COUNT=%d", new_sensor_nodes_count, MAX_SENSOR_NODES_COUNT);
            xTaskCreate(sensor_data_collection, "Monitor", 1024 * 5, NULL, 6, &monitoring_task);
            sensor_nodes_update_status = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}