#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief WiFi Manager for ESP32-S3
 * 
 * Handles:
 * - WiFi scanning
 * - Network connection
 * - WiFi status monitoring
 * - Event callbacks
 */

class WiFiManager {
public:
    // WiFi network information
    struct NetworkInfo {
        std::string ssid;
        int8_t rssi;          // Signal strength
        uint8_t channel;
        wifi_auth_mode_t auth_mode;
        bool is_hidden;
    };

    // WiFi status enum
    enum class WiFiStatus {
        DISCONNECTED = 0,
        SCANNING = 1,
        CONNECTING = 2,
        CONNECTED = 3,
        ERROR = 4,
    };

    /**
     * @brief Constructor
     */
    WiFiManager();

    /**
     * @brief Initialize WiFi stack
     * @return ESP_OK on success
     */
    esp_err_t init();

    /**
     * @brief Start WiFi scanning
     * @return ESP_OK on success
     */
    esp_err_t start_scan();

    /**
     * @brief Get list of scanned networks
     * @return Vector of available networks
     */
    std::vector<NetworkInfo> get_scan_results() const { return scan_results; }

    /**
     * @brief Connect to WiFi network
     * @param ssid Network SSID
     * @param password Network password
     * @return ESP_OK on success
     */
    esp_err_t connect(const std::string& ssid, const std::string& password);

    /**
     * @brief Disconnect from WiFi
     * @return ESP_OK on success
     */
    esp_err_t disconnect();

    /**
     * @brief Get current WiFi status
     * @return WiFiStatus enum value
     */
    WiFiStatus get_status() const { return current_status; }

    /**
     * @brief Get connected SSID
     * @return Connected SSID string
     */
    std::string get_connected_ssid() const { return connected_ssid; }

    /**
     * @brief Get signal strength (RSSI)
     * @return RSSI value in dBm
     */
    int8_t get_rssi() const { return current_rssi; }

    /**
     * @brief Check if WiFi is connected
     * @return true if connected, false otherwise
     */
    bool is_connected() const { return current_status == WiFiStatus::CONNECTED; }

    /**
     * @brief Get scan count
     * @return Number of networks found in last scan
     */
    uint16_t get_scan_count() const { return scan_results.size(); }

    /**
     * @brief Deinitialize WiFi
     * @return ESP_OK on success
     */
    esp_err_t deinit();

private:
    WiFiStatus current_status;
    std::string connected_ssid;
    int8_t current_rssi;
    std::vector<NetworkInfo> scan_results;
    bool is_scanning;

    /**
     * @brief WiFi event handler (static)
     */
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);

    /**
     * @brief Handle WiFi scan complete event
     */
    void on_scan_done();

    /**
     * @brief Handle WiFi connected event
     */
    void on_wifi_connected();

    /**
     * @brief Handle WiFi disconnected event
     */
    void on_wifi_disconnected();
};
