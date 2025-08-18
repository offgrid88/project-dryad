/**
 * @file lcd_driver.c
 * @brief GC9A01 LCD driver implementation for ESP32-C3
 */

#include <string.h>
#include <stdlib.h>
#include "lcd_driver.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Basic 5x7 font data
#include "font5x7.h"

static const char *TAG = "LCD_DRIVER";

// Pin configuration
#define LCD_HOST          SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL

// GPIO pins - adjust these according to your hardware setup
#define PIN_NUM_MOSI      7   // SDA on LCD
#define PIN_NUM_CLK       6   // SCL on LCD
#define PIN_NUM_CS        10
#define PIN_NUM_DC        8   // DC/RS on LCD
#define PIN_NUM_RST       4   // RES on LCD
#define PIN_NUM_BK_LIGHT  5   // Backlight control (optional)

// LCD dimensions
#define LCD_H_RES         240
#define LCD_V_RES         240

// LCD panel handles
static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

// Frame buffer for faster operations (optional)
static uint16_t *frame_buffer = NULL;
static size_t fb_size = 0;

/**
 * Initialize the backlight PWM
 */
static esp_err_t init_backlight(void)
{
#ifdef PIN_NUM_BK_LIGHT
    // Configure LEDC for backlight control
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_NUM_BK_LIGHT,
        .duty           = 4096, // 50% duty cycle
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
#endif
    return ESP_OK;
}

/**
 * Initialize the LCD
 */
esp_err_t lcd_init(void)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t) + 8
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install GC9A01 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // Initialize backlight
    init_backlight();
    lcd_set_brightness(100);

    // Clear screen to black
    lcd_clear(0x0000);

    ESP_LOGI(TAG, "LCD initialized successfully");
    return ret;
}

/**
 * Clear the entire screen with a color
 */
void lcd_clear(uint16_t color)
{
    if (panel_handle == NULL) return;
    
    // Create a buffer filled with the color
    size_t buffer_size = LCD_H_RES * 10; // Process in chunks
    uint16_t *buffer = heap_caps_malloc(buffer_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer for clear");
        return;
    }
    
    // Fill buffer with color
    for (size_t i = 0; i < buffer_size; i++) {
        buffer[i] = color;
    }
    
    // Send buffer to display in chunks
    for (int y = 0; y < LCD_V_RES; y += 10) {
        int height = (y + 10 > LCD_V_RES) ? (LCD_V_RES - y) : 10;
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_H_RES, y + height, buffer);
    }
    
    free(buffer);
}

/**
 * Draw a single pixel
 */
void lcd_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (panel_handle == NULL || x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES) return;
    
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, &color);
}

/**
 * Draw a line using Bresenham's algorithm
 */
void lcd_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    
    if (steep) {
        int16_t temp = x0; x0 = y0; y0 = temp;
        temp = x1; x1 = y1; y1 = temp;
    }
    
    if (x0 > x1) {
        int16_t temp = x0; x0 = x1; x1 = temp;
        temp = y0; y0 = y1; y1 = temp;
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            lcd_draw_pixel(y0, x0, color);
        } else {
            lcd_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * Draw a rectangle outline
 */
void lcd_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    lcd_draw_line(x0, y0, x1, y0, color);
    lcd_draw_line(x1, y0, x1, y1, color);
    lcd_draw_line(x1, y1, x0, y1, color);
    lcd_draw_line(x0, y1, x0, y0, color);
}

/**
 * Draw a filled rectangle
 */
void lcd_draw_filled_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if (panel_handle == NULL) return;
    
    // Ensure coordinates are in correct order
    if (x0 > x1) { int16_t temp = x0; x0 = x1; x1 = temp; }
    if (y0 > y1) { int16_t temp = y0; y0 = y1; y1 = temp; }
    
    // Clip to screen bounds
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= LCD_H_RES) x1 = LCD_H_RES - 1;
    if (y1 >= LCD_V_RES) y1 = LCD_V_RES - 1;
    
    int width = x1 - x0 + 1;
    int height = y1 - y0 + 1;
    
    // Create buffer filled with color
    size_t buffer_size = width * height;
    uint16_t *buffer = heap_caps_malloc(buffer_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (buffer == NULL) {
        // Fall back to pixel-by-pixel drawing
        for (int16_t y = y0; y <= y1; y++) {
            for (int16_t x = x0; x <= x1; x++) {
                lcd_draw_pixel(x, y, color);
            }
        }
        return;
    }
    
    // Fill buffer
    for (size_t i = 0; i < buffer_size; i++) {
        buffer[i] = color;
    }
    
    // Draw bitmap
    esp_lcd_panel_draw_bitmap(panel_handle, x0, y0, x1 + 1, y1 + 1, buffer);
    
    free(buffer);
}

/**
 * Draw a circle outline using Bresenham's algorithm
 */
void lcd_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x_pos = 0;
    int16_t y_pos = r;
    
    lcd_draw_pixel(x, y + r, color);
    lcd_draw_pixel(x, y - r, color);
    lcd_draw_pixel(x + r, y, color);
    lcd_draw_pixel(x - r, y, color);
    
    while (x_pos < y_pos) {
        if (f >= 0) {
            y_pos--;
            ddF_y += 2;
            f += ddF_y;
        }
        x_pos++;
        ddF_x += 2;
        f += ddF_x;
        
        lcd_draw_pixel(x + x_pos, y + y_pos, color);
        lcd_draw_pixel(x - x_pos, y + y_pos, color);
        lcd_draw_pixel(x + x_pos, y - y_pos, color);
        lcd_draw_pixel(x - x_pos, y - y_pos, color);
        lcd_draw_pixel(x + y_pos, y + x_pos, color);
        lcd_draw_pixel(x - y_pos, y + x_pos, color);
        lcd_draw_pixel(x + y_pos, y - x_pos, color);
        lcd_draw_pixel(x - y_pos, y - x_pos, color);
    }
}

/**
 * Draw a filled circle
 */
void lcd_draw_filled_circle(int16_t x, int16_t y, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x_pos = 0;
    int16_t y_pos = r;
    
    lcd_draw_line(x, y - r, x, y + r, color);
    
    while (x_pos < y_pos) {
        if (f >= 0) {
            y_pos--;
            ddF_y += 2;
            f += ddF_y;
        }
        x_pos++;
        ddF_x += 2;
        f += ddF_x;
        
        lcd_draw_line(x - x_pos, y - y_pos, x - x_pos, y + y_pos, color);
        lcd_draw_line(x + x_pos, y - y_pos, x + x_pos, y + y_pos, color);
        lcd_draw_line(x - y_pos, y - x_pos, x - y_pos, y + x_pos, color);
        lcd_draw_line(x + y_pos, y - x_pos, x + y_pos, y + x_pos, color);
    }
}

/**
 * Draw a character
 */
void lcd_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size)
{
    if (c < ' ' || c > '~') c = '?'; // Replace unsupported chars
    
    for (int8_t i = 0; i < 5; i++) {
        uint8_t line = font5x7[(c - ' ') * 5 + i];
        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x01) {
                if (size == 1) {
                    lcd_draw_pixel(x + i, y + j, color);
                } else {
                    lcd_draw_filled_rectangle(x + i * size, y + j * size, 
                                            x + i * size + size - 1, 
                                            y + j * size + size - 1, color);
                }
            } else if (bg_color != color) {
                if (size == 1) {
                    lcd_draw_pixel(x + i, y + j, bg_color);
                } else {
                    lcd_draw_filled_rectangle(x + i * size, y + j * size, 
                                            x + i * size + size - 1, 
                                            y + j * size + size - 1, bg_color);
                }
            }
            line >>= 1;
        }
    }
}

/**
 * Draw a string
 */
void lcd_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size)
{
    while (*str) {
        lcd_draw_char(x, y, *str, color, bg_color, size);
        x += 6 * size; // 5 pixels wide + 1 space
        str++;
    }
}

/**
 * Set display rotation
 */
void lcd_set_rotation(uint8_t rotation)
{
    if (panel_handle == NULL) return;
    
    bool mirror_x = false;
    bool mirror_y = false;
    bool swap_xy = false;
    
    switch (rotation & 3) {
        case 0:
            break;
        case 1:
            swap_xy = true;
            mirror_x = true;
            break;
        case 2:
            mirror_x = true;
            mirror_y = true;
            break;
        case 3:
            swap_xy = true;
            mirror_y = true;
            break;
    }
    
    esp_lcd_panel_mirror(panel_handle, mirror_x, mirror_y);
    esp_lcd_panel_swap_xy(panel_handle, swap_xy);
}

/**
 * Turn display on/off
 */
void lcd_display_on(bool on)
{
    if (panel_handle == NULL) return;
    
    esp_lcd_panel_disp_on_off(panel_handle, on);
}

/**
 * Set display brightness
 */
void lcd_set_brightness(uint8_t brightness)
{
#ifdef PIN_NUM_BK_LIGHT
    if (brightness > 100) brightness = 100;
    
    uint32_t duty = (8191 * brightness) / 100; // Convert to 13-bit duty
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
#endif
}