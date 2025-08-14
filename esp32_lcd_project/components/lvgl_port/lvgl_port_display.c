/**
 * @file lvgl_port_display.c
 * @brief LVGL display driver implementation for ESP32-C3 LCD
 */

#include "lvgl_port_display.h"
#include "lcd_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "LVGL_PORT";

// Display buffer size (1/10 of the screen)
#define DISP_BUF_SIZE (240 * 240 / 10)

// LVGL objects
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_disp_t *disp;
static lv_color_t *buf1;
static lv_color_t *buf2;

// Mutex for thread safety
static SemaphoreHandle_t lvgl_mutex = NULL;

// Forward declarations
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

/**
 * @brief Flush display buffer to LCD
 */
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x, y;
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // Create a temporary buffer for the rectangular area
    size_t len = w * h;
    uint16_t *buffer = (uint16_t *)color_p;
    
    // For GC9A01 circular display, we need to check if pixels are within the circle
    int32_t center_x = 120; // Display center X
    int32_t center_y = 120; // Display center Y
    int32_t radius = 120;   // Display radius
    
    // Draw the buffer pixel by pixel (not optimal, but works for circular display)
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            // Check if pixel is within the circular display
            int32_t dx = x - center_x;
            int32_t dy = y - center_y;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                // Get color from buffer
                uint16_t color = buffer[(y - area->y1) * w + (x - area->x1)];
                lcd_draw_pixel(x, y, color);
            }
        }
    }
    
    // Inform LVGL that flushing is done
    lv_disp_flush_ready(disp_drv);
}

/**
 * @brief Initialize LVGL display driver
 */
esp_err_t lvgl_port_display_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL display port");
    
    // Create mutex
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffers
    buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    if (buf1 == NULL || buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate display buffers");
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize display buffer
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &disp_buf;
    
    // Register display driver
    disp = lv_disp_drv_register(&disp_drv);
    
    ESP_LOGI(TAG, "LVGL display port initialized");
    return ESP_OK;
}

/**
 * @brief Get the active display object
 */
lv_disp_t *lvgl_port_display_get(void)
{
    return disp;
}

/**
 * @brief Lock the display mutex
 */
bool lvgl_port_lock(int timeout_ms)
{
    if (lvgl_mutex == NULL) {
        return false;
    }
    
    const TickType_t timeout_ticks = timeout_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mutex, timeout_ticks) == pdTRUE;
}

/**
 * @brief Unlock the display mutex
 */
void lvgl_port_unlock(void)
{
    if (lvgl_mutex != NULL) {
        xSemaphoreGive(lvgl_mutex);
    }
}