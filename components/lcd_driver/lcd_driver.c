/**
 * @file lcd_driver.c
 * @brief ST7789 via i8080 8-bit parallel interface — WT32S3-28S PRO
 *
 * Pin mapping from datasheet Table 3:
 *   BL_PWM   → GPIO 47   (backlight PWM, high = on)
 *   LCD_RESET→ GPIO  3   (shared with touch reset)
 *   LCD_RS   → GPIO 18   (DC: low = cmd, high = data)
 *   LCD_WR   → GPIO 17   (write strobe)
 *   DB0..DB7 → GPIO 16,40,15,7,41,42,2,1
 */

#include <string.h>
#include <math.h>
#include "lcd_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LCD_DRIVER";

/* ============================================================
 * ESP32-S3 — full i8080 implementation
 * ============================================================ */
#if CONFIG_IDF_TARGET_ESP32S3

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "rom/ets_sys.h"

/* WT32S3-28S PRO verified pin assignments */
#define PIN_BL       47
#define PIN_RST       3
#define PIN_DC       18   /* LCD_RS = DC/RS */
#define PIN_WR       17   /* LCD_WR = write strobe */
#define PIN_DB0      16
#define PIN_DB1      40
#define PIN_DB2      15
#define PIN_DB3       7
#define PIN_DB4      41
#define PIN_DB5      42
#define PIN_DB6       2
#define PIN_DB7       1

#define LCD_H_RES    240
#define LCD_V_RES    320
#define LCD_CLK_HZ   (5 * 1000 * 1000)

static esp_lcd_i80_bus_handle_t  s_i80_bus     = NULL;
static esp_lcd_panel_io_handle_t s_io_handle   = NULL;
static esp_lcd_panel_handle_t    s_panel       = NULL;

static void backlight_init(void)
{
    /* Drive backlight GPIO high directly — simpler than LEDC and easier to debug */
    gpio_config_t bl = {
        .pin_bit_mask = (1ULL << PIN_BL),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl);
    gpio_set_level(PIN_BL, 1);
    ESP_LOGI(TAG, "Backlight on GPIO %d (HIGH)", PIN_BL);
}

esp_err_t lcd_init(void)
{
    ESP_LOGI(TAG, "Init i8080 8-bit parallel bus");

    esp_lcd_i80_bus_config_t bus_cfg = {
        .clk_src     = LCD_CLK_SRC_DEFAULT,
        .wr_gpio_num = PIN_WR,
        .dc_gpio_num = PIN_DC,
        .bus_width   = 8,
        .data_gpio_nums = {
            PIN_DB0, PIN_DB1, PIN_DB2, PIN_DB3,
            PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7,
        },
        .max_transfer_bytes = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_cfg, &s_i80_bus));

    esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num       = -1,   /* CS tied low on the board */
        .pclk_hz           = LCD_CLK_HZ,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level  = 0,
            .dc_cmd_level   = 0,
            .dc_dummy_level = 0,
            .dc_data_level  = 1,
        },
        .flags = {
            /* swap_color_bytes=0: DMA sends bytes in memory order (LSB first).
             * data_endian=LITTLE (see panel_cfg below) tells the ST7789 via
             * RAMCTRL to expect LSB first, so no software swap is needed. */
            .swap_color_bytes = 0,
        },
        .lcd_cmd_bits   = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(s_i80_bus, &io_cfg, &s_io_handle));

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_RST,
        .rgb_endian     = LCD_RGB_ENDIAN_RGB,
        /* Tell ST7789 (via RAMCTRL) the host sends LSB first.
         * The ESP32-S3 DMA sends buffer bytes in address order (low addr = low byte = LSB first).
         * Default is BIG endian (MSB first) which mismatches — this fixes it. */
        .data_endian    = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io_handle, &panel_cfg, &s_panel));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(s_panel, 0, 0));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    backlight_init();
    /* Do NOT call lcd_clear here — its async DMA transfers would fire on_trans_done
     * after the LVGL semaphore callback is registered, creating a stale give that
     * causes disp_flush to skip the first tile's synchronisation.
     * LVGL renders its own background immediately after init. */
    ESP_LOGI(TAG, "LCD ready (%dx%d)", LCD_H_RES, LCD_V_RES);
    return ESP_OK;
}

void lcd_draw_bitmap(int x1, int y1, int x2, int y2, const void *data)
{
    esp_lcd_panel_draw_bitmap(s_panel, x1, y1, x2, y2, data);
}

void lcd_clear(uint16_t color)
{
    if (!s_panel) return;
    uint16_t *buf = heap_caps_malloc(LCD_H_RES * 10 * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buf) return;
    for (int i = 0; i < LCD_H_RES * 10; i++) buf[i] = color;
    for (int y = 0; y < LCD_V_RES; y += 10) {
        int h = (y + 10 > LCD_V_RES) ? (LCD_V_RES - y) : 10;
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, LCD_H_RES, y + h, buf);
    }
    free(buf);
}

void lcd_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (!s_panel) return;
    esp_lcd_panel_draw_bitmap(s_panel, x, y, x + 1, y + 1, &color);
}

void lcd_set_brightness(uint8_t pct)
{
    gpio_set_level(PIN_BL, pct > 0 ? 1 : 0);
}

void lcd_display_on(bool on)
{
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, on);
}

void lcd_set_rotation(uint8_t r)
{
    if (!s_panel) return;
    esp_lcd_panel_swap_xy(s_panel,   r == 1 || r == 3);
    esp_lcd_panel_mirror(s_panel,    r == 2 || r == 3, r == 1 || r == 2);
}

esp_lcd_panel_io_handle_t lcd_get_io_handle(void) { return s_io_handle; }

/* ============================================================
 * Non-S3 targets — stub implementations (no i8080 hardware)
 * ============================================================ */
#else

esp_err_t lcd_init(void)                                             { return ESP_OK; }
void lcd_draw_bitmap(int x1, int y1, int x2, int y2, const void *d) { (void)x1;(void)y1;(void)x2;(void)y2;(void)d; }
void lcd_clear(uint16_t color)                                       { (void)color; }
void lcd_draw_pixel(int16_t x, int16_t y, uint16_t c)               { (void)x;(void)y;(void)c; }
void lcd_set_brightness(uint8_t b)                                   { (void)b; }
void lcd_display_on(bool on)                                         { (void)on; }
void lcd_set_rotation(uint8_t r)                                     { (void)r; }
esp_lcd_panel_io_handle_t lcd_get_io_handle(void)                   { return NULL; }

#endif  /* CONFIG_IDF_TARGET_ESP32S3 */

/* ============================================================
 * Higher-level drawing (target-independent, built on draw_pixel)
 * These are slow — only use outside of LVGL rendering.
 * ============================================================ */

void lcd_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { int16_t t; t=x0; x0=y0; y0=t; t=x1; x1=y1; y1=t; }
    if (x0 > x1) { int16_t t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    int16_t dx = x1 - x0, dy = abs(y1 - y0);
    int16_t err = dx / 2, ystep = (y0 < y1) ? 1 : -1, y = y0;
    for (int16_t x = x0; x <= x1; x++) {
        if (steep) lcd_draw_pixel(y, x, color); else lcd_draw_pixel(x, y, color);
        err -= dy;
        if (err < 0) { y += ystep; err += dx; }
    }
}

void lcd_draw_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    lcd_draw_line(x0, y0, x1, y0, color);
    lcd_draw_line(x1, y0, x1, y1, color);
    lcd_draw_line(x0, y1, x1, y1, color);
    lcd_draw_line(x0, y0, x0, y1, color);
}

void lcd_draw_filled_rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    for (int16_t y = y0; y <= y1; y++)
        lcd_draw_line(x0, y, x1, y, color);
}

void lcd_draw_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
{
    int16_t x = 0, y = r, d = 1 - r;
    while (x <= y) {
        lcd_draw_pixel(cx+x, cy+y, color); lcd_draw_pixel(cx-x, cy+y, color);
        lcd_draw_pixel(cx+x, cy-y, color); lcd_draw_pixel(cx-x, cy-y, color);
        lcd_draw_pixel(cx+y, cy+x, color); lcd_draw_pixel(cx-y, cy+x, color);
        lcd_draw_pixel(cx+y, cy-x, color); lcd_draw_pixel(cx-y, cy-x, color);
        if (d < 0) d += 2 * x + 3; else { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

void lcd_draw_filled_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
{
    for (int16_t y = -r; y <= r; y++) {
        int16_t w = (int16_t)sqrt((float)(r*r - y*y));
        lcd_draw_line(cx - w, cy + y, cx + w, cy + y, color);
    }
}

/* Char/string drawing omitted — not needed for LVGL. */
void lcd_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg, uint8_t sz) { (void)x;(void)y;(void)c;(void)color;(void)bg;(void)sz; }
void lcd_draw_string(int16_t x, int16_t y, const char *s, uint16_t color, uint16_t bg, uint8_t sz) { (void)x;(void)y;(void)s;(void)color;(void)bg;(void)sz; }
