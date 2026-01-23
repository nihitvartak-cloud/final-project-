#include "display_driver.hpp"
#include "wifi_manager.hpp"
#include "time_manager.hpp"
#include "ai_logic.hpp"
#include "ui_manager.hpp"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "main";

// Pin configuration for ESP32-S3 (customize based on your board)
DisplayDriver::SPIPins SPI_PINS = {
    .sclk = GPIO_NUM_8,      // SCK
    .mosi = GPIO_NUM_3,      // MOSI
    .miso = GPIO_NUM_46,     // MISO
    .cs = GPIO_NUM_9,        // CS
    .dc = GPIO_NUM_47,       // DC (Data/Command)
    .reset = GPIO_NUM_48,    // Reset
    .backlight = GPIO_NUM_5, // Backlight (PWM capable)
};

// Global manager instances
static DisplayDriver* g_display_driver = nullptr;
static WiFiManager* g_wifi_manager = nullptr;
static TimeManager* g_time_manager = nullptr;
static AILogic* g_ai_logic = nullptr;
static UIManager* g_ui_manager = nullptr;

/**
 * @brief Initialize backlight PWM
 */
static esp_err_t init_backlight_pwm() {
    if (SPI_PINS.backlight == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "Backlight GPIO not configured");
        return ESP_OK;
    }

    // Configure LEDC for backlight PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000, // 5 kHz
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };

    ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), TAG, "Failed to config LEDC timer");

    ledc_channel_config_t ledc_channel = {
        .gpio_num = SPI_PINS.backlight,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 8191, // Full brightness (13-bit max)
        .hpoint = 0,
        .flags = {
            .output_invert = 0,
        },
    };

    ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_channel), TAG, "Failed to config LEDC channel");

    ESP_LOGI(TAG, "Backlight PWM initialized");
    return ESP_OK;
}

/**
 * @brief LVGL task - handles rendering and input
 */
static void lv_task(void* arg) {
    while (1) {
        // Handle LVGL tasks
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/**
 * @brief Application demo task - shows interaction
 */
static void app_demo_task(void* arg) {
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Demo: Simulate user interactions
    ESP_LOGI(TAG, "Starting demo interactions...");

    // Demo 1: AI Assistance interaction
    if (g_ai_logic) {
        ESP_LOGI(TAG, "Demo: Activating AI listening");
        g_ai_logic->activate_listening();
        vTaskDelay(pdMS_TO_TICKS(500));

        g_ai_logic->process_input("Hello, what time is it?");
        vTaskDelay(pdMS_TO_TICKS(1000));

        g_ai_logic->speak_response(g_ai_logic->get_last_response());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Demo 2: WiFi scan
    if (g_wifi_manager) {
        ESP_LOGI(TAG, "Demo: Starting WiFi scan");
        g_wifi_manager->start_scan();
        vTaskDelay(pdMS_TO_TICKS(5000));

        uint16_t scan_count = g_wifi_manager->get_scan_count();
        ESP_LOGI(TAG, "Found %d WiFi networks", scan_count);
    }

    // Demo 3: Screen navigation
    if (g_ui_manager) {
        ESP_LOGI(TAG, "Demo: Navigating screens");
        vTaskDelay(pdMS_TO_TICKS(3000));

        g_ui_manager->handle_gesture(UIManager::GestureType::SWIPE_LEFT);
        vTaskDelay(pdMS_TO_TICKS(2000));

        g_ui_manager->handle_gesture(UIManager::GestureType::SWIPE_LEFT);
        vTaskDelay(pdMS_TO_TICKS(2000));

        g_ui_manager->handle_gesture(UIManager::GestureType::CLICK);
        vTaskDelay(pdMS_TO_TICKS(2000));

        g_ui_manager->handle_gesture(UIManager::GestureType::SWIPE_RIGHT);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGI(TAG, "Demo completed. System ready for use.");

    vTaskDelete(nullptr);
}

/**
 * @brief Application entry point
 */
extern "C" void app_main() {
    ESP_LOGI(TAG, "=======================================================");
    ESP_LOGI(TAG, "   ESP32-S3 Smart Assistant - UI & Display System");
    ESP_LOGI(TAG, "=======================================================");

    // Initialize NVS (required for WiFi and other components)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize backlight PWM
    ESP_LOGI(TAG, "Initializing backlight PWM...");
    ESP_ERROR_CHECK(init_backlight_pwm());

    // === Initialize Display Driver ===
    ESP_LOGI(TAG, "Initializing display driver...");
    g_display_driver = new DisplayDriver(SPI_PINS);
    if (g_display_driver->init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize display driver");
        return;
    }
    ESP_LOGI(TAG, "✓ Display driver initialized");

    // === Initialize WiFi Manager ===
    ESP_LOGI(TAG, "Initializing WiFi manager...");
    g_wifi_manager = new WiFiManager();
    if (g_wifi_manager->init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi manager");
        return;
    }
    ESP_LOGI(TAG, "✓ WiFi manager initialized");

    // === Initialize Time Manager ===
    ESP_LOGI(TAG, "Initializing time manager...");
    g_time_manager = new TimeManager();
    if (g_time_manager->init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize time manager");
        return;
    }
    // Set timezone to IST (UTC+5:30)
    g_time_manager->set_timezone(19800, "IST");
    ESP_LOGI(TAG, "✓ Time manager initialized");

    // === Initialize AI Logic ===
    ESP_LOGI(TAG, "Initializing AI logic...");
    g_ai_logic = new AILogic();
    if (g_ai_logic->init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize AI logic");
        return;
    }
    ESP_LOGI(TAG, "✓ AI logic initialized");

    // === Initialize UI Manager ===
    ESP_LOGI(TAG, "Initializing UI manager...");
    g_ui_manager = new UIManager(g_display_driver, g_wifi_manager, g_time_manager, g_ai_logic);
    if (g_ui_manager->init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UI manager");
        return;
    }
    if (g_ui_manager->start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start UI manager");
        return;
    }
    ESP_LOGI(TAG, "✓ UI manager initialized");

    // Create LVGL task
    xTaskCreatePinnedToCore(
        lv_task,
        "lv_task",
        4096,
        nullptr,
        2,
        nullptr,
        1  // Pin to core 1
    );

    // Create demo task to show system capabilities
    xTaskCreatePinnedToCore(
        app_demo_task,
        "demo_task",
        2048,
        nullptr,
        1,
        nullptr,
        0  // Pin to core 0
    );

    ESP_LOGI(TAG, "=======================================================");
    ESP_LOGI(TAG, "✓ ESP32-S3 Smart Assistant System Ready!");
    ESP_LOGI(TAG, "  Display: 240x320 ILI9341 TFT LCD");
    ESP_LOGI(TAG, "  UI: LVGL with swipe navigation");
    ESP_LOGI(TAG, "  WiFi: Enabled with scanning");
    ESP_LOGI(TAG, "  Time: Synchronized via SNTP");
    ESP_LOGI(TAG, "  AI: Assistant mode active");
    ESP_LOGI(TAG, "=======================================================");

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Main application logic goes here
        // The system is event-driven, so most work happens in tasks
    }
}
