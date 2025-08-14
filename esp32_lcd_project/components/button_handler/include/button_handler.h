/**
 * @file button_handler.h
 * @brief Button handler for ESP32-C3
 * 
 * This component provides debounced button input handling with
 * support for press, release, and long press events.
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Button event types
 */
typedef enum {
    BUTTON_PRESS,      // Button pressed down
    BUTTON_RELEASE,    // Button released
    BUTTON_LONG_PRESS  // Button held for long duration
} button_event_type_t;

/**
 * Button event structure
 */
typedef struct {
    uint8_t button_num;           // Button number (0-based)
    button_event_type_t event_type; // Type of event
    uint32_t timestamp;           // Event timestamp (ms)
} button_event_t;

/**
 * Button configuration
 */
typedef struct {
    uint8_t gpio_num;             // GPIO pin number
    bool active_low;              // true if button connects to GND when pressed
    bool pull_up_en;              // Enable internal pull-up
    bool pull_down_en;            // Enable internal pull-down
    uint32_t debounce_ms;         // Debounce time in milliseconds
    uint32_t long_press_ms;       // Long press threshold in milliseconds
} button_config_t;

/**
 * @brief Initialize button handler
 * 
 * @param event_queue Queue to send button events to
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_init(QueueHandle_t event_queue);

/**
 * @brief Add a button to the handler
 * 
 * @param button_num Button number (0-based)
 * @param config Button configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_add(uint8_t button_num, const button_config_t *config);

/**
 * @brief Remove a button from the handler
 * 
 * @param button_num Button number to remove
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_remove(uint8_t button_num);

/**
 * @brief Get current state of a button
 * 
 * @param button_num Button number
 * @return true if pressed, false if released
 */
bool button_is_pressed(uint8_t button_num);

/**
 * @brief Deinitialize button handler
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_HANDLER_H