/**
 * @file lvgl_port_display.h
 * @brief LVGL display driver port for ESP32-C3 LCD
 */

#ifndef LVGL_PORT_DISPLAY_H
#define LVGL_PORT_DISPLAY_H

#include <stdbool.h>
#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL display driver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lvgl_port_display_init(void);

/**
 * @brief Get the active display object
 * 
 * @return Pointer to the display object
 */
lv_disp_t *lvgl_port_display_get(void);

/**
 * @brief Lock the display mutex (for thread safety)
 * 
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return true if locked successfully, false on timeout
 */
bool lvgl_port_lock(int timeout_ms);

/**
 * @brief Unlock the display mutex
 */
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_DISPLAY_H