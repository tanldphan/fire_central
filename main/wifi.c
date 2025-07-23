#include "wifi.h"

static const char* TAG_WIFI = "WIFI";

static int retry_count = 0;
static EventGroupHandle_t wifi_event_group;


void wifi_init()
{
    ESP_ERROR_CHECK (esp_netif_init ());
    wifi_event_group = xEventGroupCreate ();
    ESP_ERROR_CHECK (esp_event_loop_create_default ());
    esp_netif_create_default_wifi_sta ();

    const wifi_init_config_t wifi_initialization = WIFI_INIT_CONFIG_DEFAULT ();
    ESP_ERROR_CHECK (esp_wifi_init (&wifi_initialization));
    ESP_ERROR_CHECK (esp_event_handler_register (WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK (esp_event_handler_register (IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    // Configure WiFi as a station.
    // wifi_config_t wifi_cfg = { .sta = { .ssid = WIFI_SSID,
    //         #ifdef CONFIG_REGULAR_WIFI
    //             .password = "123456789", // <<<<<<<<<<<<!!!
    //             .scan_method = WIFI_ALL_CHANNEL_SCAN,
    //             .threshold =
    //             {
    //                 .rssi = -127,
    //                 .authmode = WIFI_AUTH_WPA2_PSK,
    //             },
    //             .pmf_cfg =
    //             {
    //                 .capable = true,
    //                 .required = false,
    //             }
    //         #endif
    //    }
    // };

    wifi_config_t wifi_cfg = { 0 };  // Zero-initialize the whole struct
    strcpy((char*)wifi_cfg.sta.ssid, WIFI_SSID);

    #ifdef CONFIG_REGULAR_WIFI
    strcpy((char*)wifi_cfg.sta.password, WIFI_PASSWORD);
    wifi_cfg.sta.scan_method = WIFI_FAST_SCAN;
    // wifi_cfg.sta.threshold.rssi = -127;
    // wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    // wifi_cfg.sta.pmf_cfg.capable = true;
    // wifi_cfg.sta.pmf_cfg.required = false;
    #endif

    // If using Protected EAP WiFi network.
    #ifdef CONFIG_PEAP_WIFI
        esp_eap_client_set_username ((uint8_t*) CONFIG_PEAP_WIFI_USERNAME, strlen (CONFIG_PEAP_WIFI_USERNAME));
        esp_eap_client_set_identity ((uint8_t*) PEAP_WIFI_IDENTITY, strlen (PEAP_WIFI_IDENTITY));
        esp_eap_client_set_password ((uint8_t*) WIFI_PASSWORD, strlen (WIFI_PASSWORD));
        esp_wifi_sta_enterprise_enable ();
    #endif

    // Set the WiFi mode as a station.
    ESP_ERROR_CHECK (esp_wifi_set_mode (WIFI_MODE_STA));
    ESP_ERROR_CHECK (esp_wifi_set_config (WIFI_IF_STA, &wifi_cfg));
    ESP_LOGI(TAG_WIFI, "SSID: %s, PASS: %s", wifi_cfg.sta.ssid, wifi_cfg.sta.password);
    ESP_ERROR_CHECK (esp_wifi_start ());

    ESP_LOGI (TAG_WIFI, "Initializing wifi station finished.");
    // Wait until connection is established (WIFI_CONNECTED_BIT) or connection
    // failed for the maximum number of retries (WIFI_FAIL_BIT). Bits are set by
    // wifi_event_handler.
    EventBits_t bits = xEventGroupWaitBits (wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
                                            pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI (TAG_WIFI, "Connected to access point SSID:%s, password:%s", WIFI_SSID, WIFI_PASSWORD);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE (TAG_WIFI, "Failed to connect to SSID:%s, password:%s", WIFI_SSID, WIFI_PASSWORD);
    }
    else
    {
        ESP_LOGE (TAG_WIFI, "UNEXPECTED EVENT.");
    }
}


// WiFi event handler. Monitors events and acts accordingly based on the statements below.
void wifi_event_handler (void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI (TAG_WIFI, "WiFi Station Started.");
        // Once the WiFi station is setup and started, try connecting to the network.
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI (TAG_WIFI, "WiFi Station Connected.");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG_WIFI, "Disconnected. Reason: %d", disconn->reason);
        if (retry_count < MAXIMUM_RETRY)
        {
            esp_wifi_connect ();
            retry_count++;
            ESP_LOGI (TAG_WIFI, "Retry to connect to access point.");
        }
        else
        {
            xEventGroupSetBits (wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE (TAG_WIFI, "Failed to connect to access point in under %d retry.", MAXIMUM_RETRY);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // If we get an IP from the access point (meaning we successfuly
        // connected to the WiFi network), print out our given IP address
        // and set the connected event bit (flag) in the wifi event group.
        ESP_LOGI (TAG_WIFI, "Connected to AP SSID: %s", WIFI_SSID);
        ip_event_got_ip_t* got_ip = (ip_event_got_ip_t*) event_data;
        ESP_LOGI (TAG_WIFI, "Got IP:" IPSTR, IP2STR (&got_ip->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits (wifi_event_group, WIFI_CONNECTED_BIT);
    }
}