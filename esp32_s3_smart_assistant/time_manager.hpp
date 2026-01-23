#pragma once

#include <ctime>
#include <string>
#include "esp_err.h"

/**
 * @brief Time Manager for ESP32-S3
 * 
 * Handles:
 * - RTC (Real-Time Clock) initialization
 * - SNTP (Simple Network Time Protocol) synchronization
 * - Time zone management
 * - Time formatting
 */

class TimeManager {
public:
    // Time structure for easy access
    struct TimeInfo {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        int day_of_week; // 0 = Sunday, 6 = Saturday
    };

    /**
     * @brief Constructor
     */
    TimeManager();

    /**
     * @brief Initialize time manager
     * @return ESP_OK on success
     */
    esp_err_t init();

    /**
     * @brief Start SNTP synchronization
     * @param server_name NTP server address (default: pool.ntp.org)
     * @return ESP_OK on success
     */
    esp_err_t start_sntp(const char* server_name = "pool.ntp.org");

    /**
     * @brief Check if time is synchronized
     * @return true if time is valid, false otherwise
     */
    bool is_time_synced() const;

    /**
     * @brief Wait for time to be synchronized
     * @param timeout_ms Maximum time to wait in milliseconds
     * @return true if synced within timeout, false otherwise
     */
    bool wait_for_sync(uint32_t timeout_ms = 10000);

    /**
     * @brief Get current time information
     * @return TimeInfo structure with current time
     */
    TimeInfo get_time() const;

    /**
     * @brief Get current time as formatted string
     * @param format Format string (e.g., "%Y-%m-%d %H:%M:%S")
     * @return Formatted time string
     */
    std::string get_time_string(const char* format = "%H:%M:%S") const;

    /**
     * @brief Get current date as formatted string
     * @param format Format string (e.g., "%A, %B %d")
     * @return Formatted date string
     */
    std::string get_date_string(const char* format = "%A, %B %d") const;

    /**
     * @brief Get day of week name
     * @return Day name (e.g., "Monday")
     */
    std::string get_day_name() const;

    /**
     * @brief Get month name
     * @param month Month number (1-12)
     * @return Month name
     */
    std::string get_month_name(int month = -1) const;

    /**
     * @brief Set time zone
     * @param tz_offset Timezone offset in seconds (e.g., 19800 for IST +5:30)
     * @param tz_name Timezone name (e.g., "IST")
     * @return ESP_OK on success
     */
    esp_err_t set_timezone(long tz_offset, const char* tz_name);

    /**
     * @brief Set time zone using TZ environment variable format
     * @param tz_str TZ string (e.g., "IST-5:30")
     * @return ESP_OK on success
     */
    esp_err_t set_timezone_str(const char* tz_str);

    /**
     * @brief Get current unix timestamp
     * @return Unix timestamp (seconds since epoch)
     */
    time_t get_unix_time() const;

    /**
     * @brief Stop SNTP synchronization
     * @return ESP_OK on success
     */
    esp_err_t stop_sntp();

    /**
     * @brief Deinitialize time manager
     * @return ESP_OK on success
     */
    esp_err_t deinit();

private:
    bool sntp_initialized;
    std::string timezone_name;
    long timezone_offset;

    /**
     * @brief Convert month number to name
     */
    const char* month_to_string(int month) const;

    /**
     * @brief Convert day of week to name
     */
    const char* day_to_string(int day) const;
};
