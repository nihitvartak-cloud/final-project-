#include "time_manager.hpp"
#include "esp_log.h"
#include "esp_check.h"
#include "sntp.h"
#include "time.h"
#include "sys/time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "TimeManager";

TimeManager::TimeManager()
    : sntp_initialized(false), timezone_offset(0) {
}

esp_err_t TimeManager::init() {
    ESP_LOGI(TAG, "Initializing Time Manager");

    // Configure time with SNTP
    // Set default timezone to UTC first
    setenv("TZ", "UTC0", 1);
    tzset();

    ESP_LOGI(TAG, "Time Manager initialized");
    return ESP_OK;
}

esp_err_t TimeManager::start_sntp(const char* server_name) {
    if (sntp_initialized) {
        ESP_LOGW(TAG, "SNTP already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting SNTP synchronization with server: %s", server_name);

    // Initialize SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)server_name);
    sntp_init();

    sntp_initialized = true;
    return ESP_OK;
}

bool TimeManager::is_time_synced() const {
    time_t now = time(nullptr);
    struct tm timeinfo = *localtime(&now);

    // Check if time is reasonable (after year 2000)
    return timeinfo.tm_year > (2000 - 1900);
}

bool TimeManager::wait_for_sync(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Waiting for time synchronization (timeout: %d ms)", timeout_ms);

    uint32_t elapsed = 0;
    const uint32_t check_interval = 100; // Check every 100ms

    while (elapsed < timeout_ms) {
        if (is_time_synced()) {
            ESP_LOGI(TAG, "Time synchronized successfully");
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(check_interval));
        elapsed += check_interval;
    }

    ESP_LOGW(TAG, "Time synchronization timeout");
    return false;
}

TimeManager::TimeInfo TimeManager::get_time() const {
    time_t now = time(nullptr);
    struct tm timeinfo = *localtime(&now);

    TimeInfo info = {
        .year = timeinfo.tm_year + 1900,
        .month = timeinfo.tm_mon + 1,
        .day = timeinfo.tm_mday,
        .hour = timeinfo.tm_hour,
        .minute = timeinfo.tm_min,
        .second = timeinfo.tm_sec,
        .day_of_week = timeinfo.tm_wday,
    };

    return info;
}

std::string TimeManager::get_time_string(const char* format) const {
    time_t now = time(nullptr);
    struct tm timeinfo = *localtime(&now);

    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);

    return std::string(buffer);
}

std::string TimeManager::get_date_string(const char* format) const {
    time_t now = time(nullptr);
    struct tm timeinfo = *localtime(&now);

    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);

    return std::string(buffer);
}

std::string TimeManager::get_day_name() const {
    TimeInfo info = get_time();
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    return days[info.day_of_week];
}

std::string TimeManager::get_month_name(int month) const {
    if (month == -1) {
        TimeInfo info = get_time();
        month = info.month;
    }

    const char* months[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    if (month >= 1 && month <= 12) {
        return months[month - 1];
    }

    return "Unknown";
}

esp_err_t TimeManager::set_timezone(long tz_offset, const char* tz_name) {
    ESP_LOGI(TAG, "Setting timezone: %s (offset: %ld seconds)", tz_name, tz_offset);

    timezone_offset = tz_offset;
    timezone_name = tz_name;

    // Calculate hours and minutes offset
    long hours = tz_offset / 3600;
    long minutes = (tz_offset % 3600) / 60;

    // Create TZ string for setenv
    // Format: "UTC+/-HH:MM"
    char tz_str[32];
    if (tz_offset >= 0) {
        snprintf(tz_str, sizeof(tz_str), "UTC-%ld:%02ld", hours, minutes);
    } else {
        snprintf(tz_str, sizeof(tz_str), "UTC+%ld:%02ld", -hours, -minutes);
    }

    setenv("TZ", tz_str, 1);
    tzset();

    ESP_LOGI(TAG, "Timezone set to: %s", tz_str);
    return ESP_OK;
}

esp_err_t TimeManager::set_timezone_str(const char* tz_str) {
    ESP_LOGI(TAG, "Setting timezone string: %s", tz_str);

    setenv("TZ", tz_str, 1);
    tzset();

    timezone_name = tz_str;
    return ESP_OK;
}

time_t TimeManager::get_unix_time() const {
    return time(nullptr);
}

esp_err_t TimeManager::stop_sntp() {
    if (sntp_initialized) {
        ESP_LOGI(TAG, "Stopping SNTP");
        sntp_stop();
        sntp_initialized = false;
    }

    return ESP_OK;
}

esp_err_t TimeManager::deinit() {
    ESP_LOGI(TAG, "Deinitializing Time Manager");

    stop_sntp();

    return ESP_OK;
}

const char* TimeManager::month_to_string(int month) const {
    const char* months[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    if (month >= 1 && month <= 12) {
        return months[month - 1];
    }

    return "Unknown";
}

const char* TimeManager::day_to_string(int day) const {
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    if (day >= 0 && day <= 6) {
        return days[day];
    }

    return "Unknown";
}
