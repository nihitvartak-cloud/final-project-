#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"

/**
 * @brief Display Driver for 2.4" ILI9341 TFT LCD on ESP32-S3
 * 
 * Hardware Configuration:
 * - Display: 2.4 inch TFT LCD
 * - Resolution: 240 × 320 (Portrait)
 * - Interface: SPI
 * - Driver IC: ILI9341
 * - Color Depth: 16-bit RGB565
 * - MCU: ESP32-S3
 */

class DisplayDriver {
public:
    // Display Specifications
    static constexpr uint16_t DISPLAY_WIDTH = 240;
    static constexpr uint16_t DISPLAY_HEIGHT = 320;
    static constexpr uint16_t BITS_PER_PIXEL = 16;

    // SPI Pin Configuration (Customizable for different ESP32-S3 boards)
    struct SPIPins {
        gpio_num_t sclk;    // Serial Clock
        gpio_num_t mosi;    // Master Out Slave In
        gpio_num_t miso;    // Master In Slave Out
        gpio_num_t cs;      // Chip Select
        gpio_num_t dc;      // Data/Command
        gpio_num_t reset;   // Reset
        gpio_num_t backlight; // Backlight
    };

    /**
     * @brief Constructor
     * @param pins SPI pin configuration
     */
    explicit DisplayDriver(const SPIPins& pins);

    /**
     * @brief Initialize the display and LVGL
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t init();

    /**
     * @brief Deinitialize the display
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t deinit();

    /**
     * @brief Get LVGL display object
     * @return Pointer to LVGL display object
     */
    lv_disp_t* get_display() const { return lv_display; }

    /**
     * @brief Flush display buffer to ILI9341
     * @param x1 Start X coordinate
     * @param y1 Start Y coordinate
     * @param x2 End X coordinate
     * @param y2 End Y coordinate
     * @param color_map Pointer to color data
     */
    void flush_display(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

    /**
     * @brief Set backlight brightness
     * @param brightness 0-255
     */
    void set_backlight(uint8_t brightness);

    /**
     * @brief Enable/disable backlight
     * @param enable true to enable, false to disable
     */
    void enable_backlight(bool enable);

private:
    // SPI and LCD Panel handles
    spi_device_handle_t spi_handle;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t panel_handle;

    // LVGL display object
    lv_disp_t* lv_display;

    // Pin configuration
    SPIPins pins_config;

    // LVGL buffer for rendering
    uint8_t* lv_buffer;
    static constexpr size_t LV_BUFFER_SIZE = DISPLAY_WIDTH * DISPLAY_HEIGHT * (BITS_PER_PIXEL / 8) / 2;

    /**
     * @brief Initialize SPI bus and device
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t init_spi_bus();

    /**
     * @brief Initialize LCD panel I/O interface
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t init_lcd_panel_io();

    /**
     * @brief Initialize ILI9341 panel
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t init_ili9341_panel();

    /**
     * @brief Initialize LVGL
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t init_lvgl();

    /**
     * @brief Callback function for SPI transaction
     * @param trans Pointer to SPI transaction
     */
    static void spi_transaction_callback(spi_transaction_t* trans);

    /**
     * @brief LVGL flush callback (static wrapper)
     * @param disp LVGL display object
     * @param area Area to flush
     * @param color_map Color data
     */
    static void lv_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

    /**
     * @brief Convert brightness to PWM duty
     * @param brightness 0-255
     * @return PWM duty value
     */
    uint32_t brightness_to_pwm_duty(uint8_t brightness);
};
