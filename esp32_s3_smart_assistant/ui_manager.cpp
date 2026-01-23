#include "ui_manager.hpp"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cstdio>

static const char* TAG = "UIManager";

// Global UI manager instance for callbacks
static UIManager* g_ui_manager_instance = nullptr;

UIManager::UIManager(DisplayDriver* display, WiFiManager* wifi, TimeManager* time, AILogic* ai)
    : display_driver(display), wifi_manager(wifi), time_manager(time), ai_logic(ai),
      current_screen(ScreenID::AI_ASSISTANCE), previous_screen(ScreenID::AI_ASSISTANCE),
      needs_refresh(true), last_x(0), last_y(0), last_touch_time(0), is_swiping(false) {
}

esp_err_t UIManager::init() {
    ESP_LOGI(TAG, "Initializing UI Manager");

    if (!display_driver) {
        ESP_LOGE(TAG, "Display driver not provided");
        return ESP_ERR_INVALID_ARG;
    }

    g_ui_manager_instance = this;

    // Create all screens
    ESP_RETURN_ON_ERROR(create_screens(), TAG, "Failed to create screens");

    ESP_LOGI(TAG, "UI Manager initialized");
    return ESP_OK;
}

esp_err_t UIManager::start() {
    ESP_LOGI(TAG, "Starting UI Manager");

    // Create UI update task
    xTaskCreatePinnedToCore(
        ui_update_task,
        "ui_update",
        4096,
        this,
        2,
        nullptr,
        1  // Pin to core 1
    );

    // Load initial screen
    navigate_to(ScreenID::AI_ASSISTANCE);

    return ESP_OK;
}

void UIManager::navigate_to(ScreenID screen_id) {
    ESP_LOGI(TAG, "Navigating to screen: %d", (int)screen_id);

    if (screen_id == current_screen) {
        return;
    }

    previous_screen = current_screen;
    current_screen = screen_id;
    needs_refresh = true;
}

void UIManager::handle_gesture(GestureType gesture) {
    ESP_LOGD(TAG, "Gesture detected: %d", (int)gesture);

    switch (gesture) {
        case GestureType::SWIPE_LEFT: {
            // Navigate to next screen (infinite loop)
            int next = (int)current_screen + 1;
            if (next > (int)ScreenID::WIFI) {
                next = (int)ScreenID::AI_ASSISTANCE;
            }
            navigate_to((ScreenID)next);
            break;
        }

        case GestureType::SWIPE_RIGHT: {
            // Navigate to previous screen (infinite loop)
            int prev = (int)current_screen - 1;
            if (prev < (int)ScreenID::AI_ASSISTANCE) {
                prev = (int)ScreenID::WIFI;
            }
            navigate_to((ScreenID)prev);
            break;
        }

        case GestureType::CLICK: {
            // Click on current screen icon - open detail screen
            switch (current_screen) {
                case ScreenID::AI_ASSISTANCE:
                    navigate_to(ScreenID::AI_STATE);
                    break;
                case ScreenID::DATE_TIME:
                    navigate_to(ScreenID::DATE_TIME_CLOCK);
                    break;
                case ScreenID::WIFI:
                    navigate_to(ScreenID::WIFI_SCAN);
                    break;
                default:
                    // Return to main screen on detail screen click
                    navigate_to((ScreenID)((int)current_screen - 10));
                    break;
            }
            break;
        }

        default:
            break;
    }
}

esp_err_t UIManager::create_screens() {
    ESP_LOGI(TAG, "Creating UI screens");

    // Create main screens
    create_main_screens();

    // Create detail screens
    create_detail_screens();

    ESP_LOGI(TAG, "All screens created successfully");
    return ESP_OK;
}

void UIManager::create_main_screens() {
    screen_ai_assistance = create_ai_assistance_screen();
    screen_date_time = create_date_time_screen();
    screen_wifi = create_wifi_screen();
}

void UIManager::create_detail_screens() {
    screen_ai_state = create_ai_state_screen();
    screen_date_time_clock = create_date_time_clock_screen();
    screen_wifi_scan = create_wifi_scan_screen();
}

lv_obj_t* UIManager::create_ai_assistance_screen() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

    // Title
    lv_obj_t* title = create_nav_bar(scr, "AI Assistant");

    // AI Icon/Status indicator
    lv_obj_t* status_box = lv_obj_create(scr);
    lv_obj_set_size(status_box, 200, 100);
    lv_obj_align(status_box, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(status_box, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_color(status_box, lv_color_hex(0x89b4fa), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_box, 2, LV_PART_MAIN);

    label_ai_state = lv_label_create(status_box);
    lv_label_set_text(label_ai_state, "AI: Idle");
    lv_obj_align(label_ai_state, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_text_color(label_ai_state, lv_color_hex(0x89b4fa), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_ai_state, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t* hint = lv_label_create(status_box);
    lv_label_set_text(hint, "Tap to interact");
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_text_color(hint, lv_color_hex(0xa6e3a1), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, LV_PART_MAIN);

    // Navigation hint
    lv_obj_t* nav_hint = lv_label_create(scr);
    lv_label_set_text(nav_hint, "← Swipe for WiFi  |  Time →");
    lv_obj_align(nav_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(nav_hint, lv_color_hex(0x6c7086), LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_hint, &lv_font_montserrat_12, LV_PART_MAIN);

    return scr;
}

lv_obj_t* UIManager::create_date_time_screen() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

    // Title
    create_nav_bar(scr, "Date & Time");

    // Time display
    lv_obj_t* time_box = lv_obj_create(scr);
    lv_obj_set_size(time_box, 200, 100);
    lv_obj_align(time_box, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(time_box, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_color(time_box, lv_color_hex(0xf38ba8), LV_PART_MAIN);
    lv_obj_set_style_border_width(time_box, 2, LV_PART_MAIN);

    label_time_display = lv_label_create(time_box);
    lv_label_set_text(label_time_display, "00:00:00");
    lv_obj_align(label_time_display, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_text_color(label_time_display, lv_color_hex(0xf38ba8), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_time_display, &lv_font_montserrat_28, LV_PART_MAIN);

    lv_obj_t* date_label = lv_label_create(time_box);
    lv_label_set_text(date_label, "Loading...");
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xa6e3a1), LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, LV_PART_MAIN);

    // Navigation hint
    lv_obj_t* nav_hint = lv_label_create(scr);
    lv_label_set_text(nav_hint, "← AI Assistant  |  WiFi →");
    lv_obj_align(nav_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(nav_hint, lv_color_hex(0x6c7086), LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_hint, &lv_font_montserrat_12, LV_PART_MAIN);

    return scr;
}

lv_obj_t* UIManager::create_wifi_screen() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

    // Title
    create_nav_bar(scr, "WiFi");

    // WiFi Status
    lv_obj_t* status_box = lv_obj_create(scr);
    lv_obj_set_size(status_box, 200, 100);
    lv_obj_align(status_box, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(status_box, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_color(status_box, lv_color_hex(0x94e2d5), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_box, 2, LV_PART_MAIN);

    label_wifi_status = lv_label_create(status_box);
    lv_label_set_text(label_wifi_status, "Disconnected");
    lv_obj_align(label_wifi_status, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_text_color(label_wifi_status, lv_color_hex(0x94e2d5), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_status, &lv_font_montserrat_20, LV_PART_MAIN);

    label_wifi_signal = lv_label_create(status_box);
    lv_label_set_text(label_wifi_signal, "Signal: N/A");
    lv_obj_align(label_wifi_signal, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_text_color(label_wifi_signal, lv_color_hex(0xa6e3a1), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_signal, &lv_font_montserrat_14, LV_PART_MAIN);

    // Navigation hint
    lv_obj_t* nav_hint = lv_label_create(scr);
    lv_label_set_text(nav_hint, "← Time  |  AI Assistant →");
    lv_obj_align(nav_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(nav_hint, lv_color_hex(0x6c7086), LV_PART_MAIN);
    lv_obj_set_style_text_font(nav_hint, &lv_font_montserrat_12, LV_PART_MAIN);

    return scr;
}

lv_obj_t* UIManager::create_ai_state_screen() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

    // Title
    create_nav_bar(scr, "AI State");

    // State info
    lv_obj_t* info_box = lv_obj_create(scr);
    lv_obj_set_size(info_box, 220, 150);
    lv_obj_align(info_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(info_box, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_color(info_box, lv_color_hex(0x89b4fa), LV_PART_MAIN);
    lv_obj_set_style_border_width(info_box, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_box, 15, LV_PART_MAIN);

    lv_obj_t* state_label = lv_label_create(info_box);
    lv_label_set_text(state_label, "State: Idle");
    lv_obj_align(state_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(state_label, lv_color_hex(0x89b4fa), LV_PART_MAIN);

    lv_obj_t* mode_label = lv_label_create(info_box);
    lv_label_set_text(mode_label, "Mode: Assistant");
    lv_obj_align(mode_label, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0xa6e3a1), LV_PART_MAIN);

    lv_obj_t* conf_label = lv_label_create(info_box);
    lv_label_set_text(conf_label, "Confidence: 0%");
    lv_obj_align(conf_label, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_obj_set_style_text_color(conf_label, lv_color_hex(0xf38ba8), LV_PART_MAIN);

    lv_obj_t* back_hint = lv_label_create(scr);
    lv_label_set_text(back_hint, "Tap to return");
    lv_obj_align(back_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(back_hint, lv_color_hex(0x6c7086), LV_PART_MAIN);

    return scr;
}

lv_obj_t* UIManager::create_date_time_clock_screen() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

    // Title
    create_nav_bar(scr, "Live Clock");

    // Large clock display
    lv_obj_t* clock_box = lv_obj_create(scr);
    lv_obj_set_size(clock_box, 200, 120);
    lv_obj_align(clock_box, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(clock_box, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_color(clock_box, lv_color_hex(0xf38ba8), LV_PART_MAIN);
    lv_obj_set_style_border_width(clock_box, 2, LV_PART_MAIN);

    lv_obj_t* clock_label = lv_label_create(clock_box);
    lv_label_set_text(clock_label, "00:00:00");
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(clock_label, lv_color_hex(0xf38ba8), LV_PART_MAIN);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_32, LV_PART_MAIN);

    lv_obj_t* back_hint = lv_label_create(scr);
    lv_label_set_text(back_hint, "Tap to return");
    lv_obj_align(back_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(back_hint, lv_color_hex(0x6c7086), LV_PART_MAIN);

    return scr;
}

lv_obj_t* UIManager::create_wifi_scan_screen() {
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

    // Title
    create_nav_bar(scr, "Available Networks");

    // Network list
    label_network_list = lv_label_create(scr);
    lv_label_set_text(label_network_list, "No networks scanned\nTap to scan");
    lv_obj_align(label_network_list, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(label_network_list, lv_color_hex(0x94e2d5), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_network_list, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_long_mode(label_network_list, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_network_list, 200);

    lv_obj_t* back_hint = lv_label_create(scr);
    lv_label_set_text(back_hint, "Tap to return");
    lv_obj_align(back_hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(back_hint, lv_color_hex(0x6c7086), LV_PART_MAIN);

    return scr;
}

lv_obj_t* UIManager::create_nav_bar(lv_obj_t* parent, const char* title) {
    lv_obj_t* nav_bar = lv_obj_create(parent);
    lv_obj_set_size(nav_bar, 240, 40);
    lv_obj_align(nav_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, lv_color_hex(0x313244), LV_PART_MAIN);
    lv_obj_set_style_border_width(nav_bar, 0, LV_PART_MAIN);

    lv_obj_t* title_label = lv_label_create(nav_bar);
    lv_label_set_text(title_label, title);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, LV_PART_MAIN);

    return nav_bar;
}

void UIManager::update_ai_state() {
    if (!label_ai_state) return;

    std::string state_text = "AI: " + ai_logic->get_state_string();
    lv_label_set_text(label_ai_state, state_text.c_str());
}

void UIManager::update_time_display() {
    if (!label_time_display) return;

    std::string time_str = time_manager->get_time_string("%H:%M:%S");
    lv_label_set_text(label_time_display, time_str.c_str());
}

void UIManager::update_wifi_display() {
    if (!label_wifi_status) return;

    if (wifi_manager->is_connected()) {
        std::string status = "Connected: " + wifi_manager->get_connected_ssid();
        lv_label_set_text(label_wifi_status, status.c_str());

        if (label_wifi_signal) {
            char signal_str[32];
            snprintf(signal_str, sizeof(signal_str), "Signal: %d dBm", wifi_manager->get_rssi());
            lv_label_set_text(label_wifi_signal, signal_str);
        }
    } else {
        lv_label_set_text(label_wifi_status, "Disconnected");
        if (label_wifi_signal) {
            lv_label_set_text(label_wifi_signal, "Signal: N/A");
        }
    }
}

void UIManager::update_displays() {
    // Update active screen based on current_screen
    switch (current_screen) {
        case ScreenID::AI_ASSISTANCE:
            update_ai_state();
            break;
        case ScreenID::DATE_TIME:
            update_time_display();
            break;
        case ScreenID::WIFI:
            update_wifi_display();
            break;
        default:
            break;
    }
}

void UIManager::ui_update_task(void* arg) {
    UIManager* manager = (UIManager*)arg;

    while (1) {
        // Check if screen needs to be loaded
        if (manager->needs_refresh) {
            manager->needs_refresh = false;

            // Load appropriate screen
            lv_obj_t* target_screen = nullptr;
            switch (manager->current_screen) {
                case ScreenID::AI_ASSISTANCE:
                    target_screen = manager->screen_ai_assistance;
                    break;
                case ScreenID::DATE_TIME:
                    target_screen = manager->screen_date_time;
                    break;
                case ScreenID::WIFI:
                    target_screen = manager->screen_wifi;
                    break;
                case ScreenID::AI_STATE:
                    target_screen = manager->screen_ai_state;
                    break;
                case ScreenID::DATE_TIME_CLOCK:
                    target_screen = manager->screen_date_time_clock;
                    break;
                case ScreenID::WIFI_SCAN:
                    target_screen = manager->screen_wifi_scan;
                    break;
                default:
                    break;
            }

            if (target_screen) {
                lv_screen_load(target_screen);
            }
        }

        // Update displays periodically
        manager->update_displays();

        vTaskDelay(pdMS_TO_TICKS(100)); // Update every 100ms
    }
}

esp_err_t UIManager::deinit() {
    ESP_LOGI(TAG, "Deinitializing UI Manager");

    g_ui_manager_instance = nullptr;

    return ESP_OK;
}
