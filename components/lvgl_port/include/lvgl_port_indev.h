/**
 * @file lvgl_port_indev.h
 * @brief LVGL input device driver port for buttons
 */

#ifndef LVGL_PORT_INDEV_H
#define LVGL_PORT_INDEV_H

#include "lvgl.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL input device driver for buttons
 * 
 * @param button_queue Queue handle for button events
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lvgl_port_indev_init(QueueHandle_t button_queue);

/**
 * @brief Get the input device object
 * 
 * @return Pointer to the input device object
 */
lv_indev_t *lvgl_port_indev_get(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_INDEV_H