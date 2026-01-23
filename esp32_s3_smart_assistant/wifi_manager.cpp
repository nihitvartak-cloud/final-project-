#include "wifi_manager.hpp"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"

static const char* TAG = "WiFiManager";

// Global WiFi manager instance for callback
static WiFiManager* g_wifi_manager_instance = nullptr;

// WiFi event group
static EventGroupHandle_t wifi_event_group = nullptr;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_SCAN_DONE_BIT = BIT1;

WiFiManager::WiFiManager()
    : current_status(WiFiStatus::DISCONNECTED),
      current_rssi(0),
      is_scanning(false) {
}

esp_err_t WiFiManager::init() {
    ESP_LOGI(TAG, "Initializing WiFi Manager");

    // Create WiFi event group
    wifi_event_group = xEventGroupCreate();
    if (!wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create WiFi event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize network interface
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to init netif");

    // Create default WiFi station netif
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed to init WiFi");

    // Register WiFi event handler
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr),
        TAG, "Failed to register WiFi event handler");

    // Register IP event handler for station mode
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, nullptr),
        TAG, "Failed to register IP event handler");

    // Set WiFi mode to station
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set WiFi mode");

    // Start WiFi
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi");

    g_wifi_manager_instance = this;

    ESP_LOGI(TAG, "WiFi Manager initialized");
    return ESP_OK;
}

esp_err_t WiFiManager::start_scan() {
    if (is_scanning) {
        ESP_LOGW(TAG, "WiFi scan already in progress");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting WiFi scan");
    current_status = WiFiStatus::SCANNING;
    is_scanning = true;

    // Clear event group
    xEventGroupClearBits(wifi_event_group, WIFI_SCAN_DONE_BIT);

    wifi_scan_config_t scan_config = {
        .ssid = nullptr,
        .bssid = nullptr,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300,
            },
        },
        .home_chan_dwell_time = {
            .scan_time = 0,
        },
    };

    ESP_RETURN_ON_ERROR(esp_wifi_scan_start(&scan_config, false), TAG, "Failed to start scan");

    return ESP_OK;
}

esp_err_t WiFiManager::connect(const std::string& ssid, const std::string& password) {
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid.c_str());

    // Clear previous results
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

    wifi_config_t wifi_config = {};

    // Copy SSID and password
    if (ssid.length() >= sizeof(wifi_config.sta.ssid)) {
        ESP_LOGE(TAG, "SSID too long");
        return ESP_ERR_INVALID_ARG;
    }

    if (password.length() >= sizeof(wifi_config.sta.password)) {
        ESP_LOGE(TAG, "Password too long");
        return ESP_ERR_INVALID_ARG;
    }

    std::memcpy(wifi_config.sta.ssid, ssid.c_str(), ssid.length());
    std::memcpy(wifi_config.sta.password, password.c_str(), password.length());

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set config");
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "Failed to connect");

    current_status = WiFiStatus::CONNECTING;
    connected_ssid = ssid;

    return ESP_OK;
}

esp_err_t WiFiManager::disconnect() {
    ESP_LOGI(TAG, "Disconnecting from WiFi");

    ESP_RETURN_ON_ERROR(esp_wifi_disconnect(), TAG, "Failed to disconnect");

    current_status = WiFiStatus::DISCONNECTED;
    connected_ssid.clear();
    current_rssi = 0;

    return ESP_OK;
}

esp_err_t WiFiManager::deinit() {
    ESP_LOGI(TAG, "Deinitializing WiFi Manager");

    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler);

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_deinit();

    if (wifi_event_group) {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = nullptr;
    }

    g_wifi_manager_instance = nullptr;
    return ESP_OK;
}

void WiFiManager::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data) {
    if (!g_wifi_manager_instance) {
        return;
    }

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                break;

            case WIFI_EVENT_SCAN_DONE: {
                ESP_LOGI(TAG, "WiFi scan complete");
                g_wifi_manager_instance->on_scan_done();
                xEventGroupSetBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
                break;
            }

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi connected");
                g_wifi_manager_instance->on_wifi_connected();
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                ESP_LOGW(TAG, "WiFi disconnected");
                g_wifi_manager_instance->on_wifi_disconnected();
                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ESP_LOGI(TAG, "Got IP address");
            g_wifi_manager_instance->current_status = WiFiStatus::CONNECTED;
        }
    }
}

void WiFiManager::on_scan_done() {
    is_scanning = false;

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if (ap_count == 0) {
        ESP_LOGW(TAG, "No networks found");
        current_status = WiFiStatus::DISCONNECTED;
        return;
    }

    // Allocate buffer for AP list
    wifi_ap_record_t* ap_list = (wifi_ap_record_t*)malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_list) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP list");
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

    // Clear previous results
    scan_results.clear();

    // Store scan results
    for (int i = 0; i < ap_count; i++) {
        NetworkInfo info = {
            .ssid = std::string((const char*)ap_list[i].ssid),
            .rssi = ap_list[i].rssi,
            .channel = ap_list[i].primary,
            .auth_mode = ap_list[i].authmode,
            .is_hidden = ap_list[i].ssid_len == 0,
        };
        scan_results.push_back(info);

        ESP_LOGI(TAG, "Found network: %s (RSSI: %d, Channel: %d)",
                 info.ssid.c_str(), info.rssi, info.channel);
    }

    free(ap_list);

    current_status = WiFiStatus::DISCONNECTED;
    ESP_LOGI(TAG, "Scan results: %d networks found", ap_count);
}

void WiFiManager::on_wifi_connected() {
    // Update RSSI when connected
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        current_rssi = ap_info.rssi;
        ESP_LOGI(TAG, "Connected - RSSI: %d dBm", current_rssi);
    }
}

void WiFiManager::on_wifi_disconnected() {
    current_status = WiFiStatus::DISCONNECTED;
    current_rssi = 0;
}
