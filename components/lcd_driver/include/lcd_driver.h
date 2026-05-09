/**
 * @file lcd_driver.h
 * @brief GC9A01 LCD driver for ESP32-C3
 * 
 * This driver provides functions to control a 240x240 circular LCD
 * with GC9A01 controller via SPI interface.
 */

#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LCD
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_init(void);

/**
 * @brief Clear the entire screen with a color
 * 
 * @param color 16-bit RGB565 color
 */
void lcd_clear(uint16_t color);

/**
 * @brief Draw a single pixel
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param color 16-bit RGB565 color
 */
void lcd_draw_pixel(int16_t x, int16_t y, uint16_t color);

/**
 * @brief Draw a line
 * 
 * @param x0 Start X coordinate
 * @param y0 Start Y coordinate
 * @param x1 End X coordinate
 * @param y1 End Y coordinate
 * @param color 16-bit RGB565 color
 */
void lcd_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * @brief Draw a rectangle outline
 * 
 * @param x0 Top-left X coordinate
 * @param y0 Top-left Y coordinate
 * @param x1 Bottom-right X coordinate
 * @param y1 Bottom-right Y coordinate
 * @param color 16-bit RGB565 color
 */
void lcd_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * @brief Draw a filled rectangle
 * 
 * @param x0 Top-left X coordinate
 * @param y0 Top-left Y coordinate
 * @param x1 Bottom-right X coordinate
 * @param y1 Bottom-right Y coordinate
 * @param color 16-bit RGB565 color
 */
void lcd_draw_filled_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * @brief Draw a circle outline
 * 
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param r Radius
 * @param color 16-bit RGB565 color
 */
void lcd_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color);

/**
 * @brief Draw a filled circle
 * 
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param r Radius
 * @param color 16-bit RGB565 color
 */
void lcd_draw_filled_circle(int16_t x, int16_t y, int16_t r, uint16_t color);

/**
 * @brief Draw a character
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param c Character to draw
 * @param color Text color (16-bit RGB565)
 * @param bg_color Background color (16-bit RGB565)
 * @param size Font size multiplier (1-3)
 */
void lcd_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size);

/**
 * @brief Draw a string
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param color Text color (16-bit RGB565)
 * @param bg_color Background color (16-bit RGB565)
 * @param size Font size multiplier (1-3)
 */
void lcd_draw_string(int16_t x, int16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size);

/**
 * @brief Set display rotation
 * 
 * @param rotation Rotation value (0-3)
 */
void lcd_set_rotation(uint8_t rotation);

/**
 * @brief Turn display on/off
 * 
 * @param on true to turn on, false to turn off
 */
void lcd_display_on(bool on);

/**
 * @brief Set display brightness (if backlight control is available)
 * 
 * @param brightness Brightness level (0-100)
 */
void lcd_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // LCD_DRIVER_H