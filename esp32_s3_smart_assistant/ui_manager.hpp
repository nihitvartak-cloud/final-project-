#pragma once

#include "display_driver.hpp"
#include "wifi_manager.hpp"
#include "time_manager.hpp"
#include "ai_logic.hpp"
#include "lvgl.h"
#include <vector>
#include <functional>

/**
 * @brief UI Manager for ESP32-S3 Smart Assistant
 * 
 * Handles:
 * - Screen management (Main + Detail screens)
 * - Swipe navigation with infinite loop
 * - UI updates (non-blocking, thread-safe)
 * - Gesture detection
 * - Animated transitions
 */

class UIManager {
public:
    // Screen IDs
    enum class ScreenID {
        AI_ASSISTANCE = 0,
        DATE_TIME = 1,
        WIFI = 2,
        // Detail screens
        AI_STATE = 10,
        DATE_TIME_CLOCK = 11,
        WIFI_SCAN = 12,
    };

    // Gesture types
    enum class GestureType {
        SWIPE_LEFT,
        SWIPE_RIGHT,
        SWIPE_UP,
        SWIPE_DOWN,
        CLICK,
        NONE,
    };

    /**
     * @brief Constructor
     * @param display Display driver instance
     * @param wifi WiFi manager instance
     * @param time Time manager instance
     * @param ai AI logic instance
     */
    UIManager(DisplayDriver* display, WiFiManager* wifi, TimeManager* time, AILogic* ai);

    /**
     * @brief Initialize UI manager
     * @return ESP_OK on success
     */
    esp_err_t init();

    /**
     * @brief Start UI update task
     * @return ESP_OK on success
     */
    esp_err_t start();

    /**
     * @brief Navigate to screen
     * @param screen_id Target screen ID
     */
    void navigate_to(ScreenID screen_id);

    /**
     * @brief Handle swipe gesture
     * @param gesture Gesture type
     */
    void handle_gesture(GestureType gesture);

    /**
     * @brief Get current screen ID
     * @return Current screen ID
     */
    ScreenID get_current_screen() const { return current_screen; }

    /**
     * @brief Trigger screen refresh
     */
    void request_refresh() { needs_refresh = true; }

    /**
     * @brief Update AI state on screen
     */
    void update_ai_state();

    /**
     * @brief Update time display
     */
    void update_time_display();

    /**
     * @brief Update WiFi status display
     */
    void update_wifi_display();

    /**
     * @brief Create all UI screens
     * @return ESP_OK on success
     */
    esp_err_t create_screens();

    /**
     * @brief Deinitialize UI manager
     * @return ESP_OK on success
     */
    esp_err_t deinit();

private:
    // Manager references
    DisplayDriver* display_driver;
    WiFiManager* wifi_manager;
    TimeManager* time_manager;
    AILogic* ai_logic;

    // Screen management
    ScreenID current_screen;
    ScreenID previous_screen;
    bool needs_refresh;

    // LVGL screen objects
    lv_obj_t* screen_ai_assistance;
    lv_obj_t* screen_date_time;
    lv_obj_t* screen_wifi;
    lv_obj_t* screen_ai_state;
    lv_obj_t* screen_date_time_clock;
    lv_obj_t* screen_wifi_scan;

    // UI element labels/objects
    lv_obj_t* label_ai_state;
    lv_obj_t* label_time_display;
    lv_obj_t* label_wifi_status;
    lv_obj_t* label_wifi_ssid;
    lv_obj_t* label_wifi_signal;
    lv_obj_t* label_network_list;

    // Touch/Gesture tracking
    int16_t last_x;
    int16_t last_y;
    uint32_t last_touch_time;
    bool is_swiping;

    /**
     * @brief Create main screens
     */
    void create_main_screens();

    /**
     * @brief Create detail screens
     */
    void create_detail_screens();

    /**
     * @brief AI Assistance main screen
     */
    lv_obj_t* create_ai_assistance_screen();

    /**
     * @brief Date & Time main screen
     */
    lv_obj_t* create_date_time_screen();

    /**
     * @brief WiFi main screen
     */
    lv_obj_t* create_wifi_screen();

    /**
     * @brief AI State detail screen
     */
    lv_obj_t* create_ai_state_screen();

    /**
     * @brief Date & Time clock detail screen
     */
    lv_obj_t* create_date_time_clock_screen();

    /**
     * @brief WiFi Scan detail screen
     */
    lv_obj_t* create_wifi_scan_screen();

    /**
     * @brief Load screen with animation
     * @param target_screen Screen to load
     */
    void load_screen_with_animation(lv_obj_t* target_screen);

    /**
     * @brief Update all active displays
     */
    void update_displays();

    /**
     * @brief Task for periodic UI updates
     */
    static void ui_update_task(void* arg);

    /**
     * @brief Create top navigation bar
     * @param parent Parent object
     * @return Navigation bar object
     */
    lv_obj_t* create_nav_bar(lv_obj_t* parent, const char* title);

    /**
     * @brief Create common style for buttons
     */
    static lv_style_t* create_button_style();

    /**
     * @brief Handle animation completion
     */
    void on_animation_complete();
};
