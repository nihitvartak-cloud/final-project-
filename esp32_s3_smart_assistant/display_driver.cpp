#include "display_driver.hpp"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <cstring>

static const char* TAG = "DisplayDriver";

// Global pointer for LVGL callback
static DisplayDriver* g_display_instance = nullptr;

DisplayDriver::DisplayDriver(const SPIPins& pins)
    : spi_handle(nullptr), io_handle(nullptr), panel_handle(nullptr),
      lv_display(nullptr), pins_config(pins), lv_buffer(nullptr) {
}

esp_err_t DisplayDriver::init() {
    ESP_LOGI(TAG, "Initializing Display Driver for ILI9341");

    ESP_RETURN_ON_ERROR(init_spi_bus(), TAG, "Failed to initialize SPI bus");
    ESP_RETURN_ON_ERROR(init_lcd_panel_io(), TAG, "Failed to initialize LCD panel I/O");
    ESP_RETURN_ON_ERROR(init_ili9341_panel(), TAG, "Failed to initialize ILI9341 panel");
    ESP_RETURN_ON_ERROR(init_lvgl(), TAG, "Failed to initialize LVGL");

    g_display_instance = this;
    ESP_LOGI(TAG, "Display Driver initialized successfully");
    return ESP_OK;
}

esp_err_t DisplayDriver::deinit() {
    ESP_LOGI(TAG, "Deinitializing Display Driver");

    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
        panel_handle = nullptr;
    }

    if (io_handle) {
        esp_lcd_panel_io_del(io_handle);
        io_handle = nullptr;
    }

    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
        spi_handle = nullptr;
    }

    if (lv_buffer) {
        free(lv_buffer);
        lv_buffer = nullptr;
    }

    g_display_instance = nullptr;
    ESP_LOGI(TAG, "Display Driver deinitialized");
    return ESP_OK;
}

esp_err_t DisplayDriver::init_spi_bus() {
    ESP_LOGI(TAG, "Initializing SPI bus");

    // Configure SPI bus
    spi_bus_config_t bus_config = {
        .mosi_io_num = pins_config.mosi,
        .miso_io_num = pins_config.miso,
        .sclk_io_num = pins_config.sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO),
                       TAG, "Failed to initialize SPI bus");

    // Configure SPI device
    spi_device_config_t device_config = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = 40 * 1000 * 1000, // 40 MHz
        .input_delay_ns = 0,
        .spics_io_num = pins_config.cs,
        .flags = 0,
        .queue_size = 7,
        .pre_cb = nullptr,
        .post_cb = nullptr,
    };

    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &device_config, &spi_handle),
                       TAG, "Failed to add SPI device");

    ESP_LOGI(TAG, "SPI bus initialized successfully");
    return ESP_OK;
}

esp_err_t DisplayDriver::init_lcd_panel_io() {
    ESP_LOGI(TAG, "Initializing LCD panel I/O");

    // Configure LCD panel I/O
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = pins_config.cs,
        .dc_gpio_num = pins_config.dc,
        .spi_mode = 0,
        .pclk_hz = 40 * 1000 * 1000, // 40 MHz
        .trans_queue_depth = 10,
        .on_color_trans_done = spi_transaction_callback,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = {
            .dc_low_on_data = 0,
            .octal_mode = 0,
            .sio_mode = 0,
            .lsb_first = 0,
            .cs_high_active = 0,
        },
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi(spi_handle, &io_config, &io_handle),
        TAG, "Failed to create LCD panel I/O");

    ESP_LOGI(TAG, "LCD panel I/O initialized successfully");
    return ESP_OK;
}

esp_err_t DisplayDriver::init_ili9341_panel() {
    ESP_LOGI(TAG, "Initializing ILI9341 panel");

    // Configure ILI9341 panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = pins_config.reset,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BITS_PER_PIXEL,
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle),
        TAG, "Failed to create ILI9341 panel");

    // Reset panel
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "Failed to reset panel");
    vTaskDelay(pdMS_TO_TICKS(100));

    // Initialize panel
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "Failed to initialize panel");
    vTaskDelay(pdMS_TO_TICKS(100));

    // Set orientation to portrait
    ESP_RETURN_ON_ERROR(
        esp_lcd_panel_swap_xy(panel_handle, false),
        TAG, "Failed to set swap_xy");

    ESP_RETURN_ON_ERROR(
        esp_lcd_panel_mirror(panel_handle, false, false),
        TAG, "Failed to set mirror");

    // Turn on display
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true),
                       TAG, "Failed to turn on display");

    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "ILI9341 panel initialized successfully");
    return ESP_OK;
}

esp_err_t DisplayDriver::init_lvgl() {
    ESP_LOGI(TAG, "Initializing LVGL");

    // Allocate LVGL buffer
    lv_buffer = (uint8_t*)malloc(LV_BUFFER_SIZE);
    if (!lv_buffer) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffer");
        return ESP_ERR_NO_MEM;
    }

    // Initialize LVGL
    lv_init();

    // Create display buffer
    lv_display_t* disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!disp) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        free(lv_buffer);
        lv_buffer = nullptr;
        return ESP_FAIL;
    }

    // Set display buffer
    lv_display_set_buffers(disp, lv_buffer, nullptr, LV_BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set flush callback
    lv_display_set_flush_cb(disp, lv_flush_cb);

    lv_display = disp;

    ESP_LOGI(TAG, "LVGL initialized successfully");
    ESP_LOGI(TAG, "Display resolution: %d x %d, Buffer size: %d bytes",
             DISPLAY_WIDTH, DISPLAY_HEIGHT, LV_BUFFER_SIZE);

    return ESP_OK;
}

void DisplayDriver::lv_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    if (g_display_instance) {
        g_display_instance->flush_display(disp, area, px_map);
    }
}

void DisplayDriver::flush_display(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    // Write pixel data to ILI9341
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);

    lv_display_flush_ready(disp);
}

void DisplayDriver::spi_transaction_callback(spi_transaction_t* trans) {
    // Callback when SPI transaction is complete
    // Can be used for synchronization if needed
}

void DisplayDriver::set_backlight(uint8_t brightness) {
    if (pins_config.backlight == GPIO_NUM_NC) {
        return;
    }

    uint32_t duty = brightness_to_pwm_duty(brightness);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void DisplayDriver::enable_backlight(bool enable) {
    if (pins_config.backlight == GPIO_NUM_NC) {
        return;
    }

    if (enable) {
        set_backlight(255);
    } else {
        set_backlight(0);
    }
}

uint32_t DisplayDriver::brightness_to_pwm_duty(uint8_t brightness) {
    // Convert 0-255 brightness to PWM duty (0-8191 for 13-bit resolution)
    return (brightness * 8191) / 255;
}
