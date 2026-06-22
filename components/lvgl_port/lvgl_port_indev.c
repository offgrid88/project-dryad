/**
 * @file lvgl_port_indev.c
 * @brief LVGL input device driver implementation for buttons
 */

#include "lvgl_port_indev.h"
#include "button_handler.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LVGL_INDEV";

// Input device objects
static lv_indev_drv_t indev_drv;
static lv_indev_t *indev_button;
static lv_group_t *default_group;

// Button queue
static QueueHandle_t btn_queue = NULL;

// Current button state
static uint32_t last_key = 0;
static lv_indev_state_t last_state = LV_INDEV_STATE_RELEASED;

// Forward declarations
static void button_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

/**
 * @brief Convert button events to LVGL key codes
 */
static uint32_t button_to_lvgl_key(uint8_t button_num)
{
    switch (button_num) {
        case 0:  // Button 1 - Navigate/Previous
            return LV_KEY_PREV;
        case 1:  // Button 2 - Select/Enter
            return LV_KEY_ENTER;
        default:
            return 0;
    }
}

/**
 * @brief Read button state for LVGL
 */
static void button_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    button_event_t event;
    
    // Check if there's a new button event
    if (xQueueReceive(btn_queue, &event, 0) == pdTRUE) {
        // Convert button number to LVGL key
        uint32_t key = button_to_lvgl_key(event.button_num);
        
        if (key != 0) {
            last_key = key;
            
            // Update state based on event type
            switch (event.event_type) {
                case BUTTON_PRESS:
                    last_state = LV_INDEV_STATE_PRESSED;
                    ESP_LOGD(TAG, "Button %d pressed -> Key %d", event.button_num, key);
                    break;
                    
                case BUTTON_RELEASE:
                    last_state = LV_INDEV_STATE_RELEASED;
                    ESP_LOGD(TAG, "Button %d released -> Key %d", event.button_num, key);
                    break;
                    
                case BUTTON_LONG_PRESS:
                    // For long press, we can optionally trigger a different action
                    // For now, treat it as a regular press
                    last_state = LV_INDEV_STATE_PRESSED;
                    ESP_LOGD(TAG, "Button %d long press -> Key %d", event.button_num, key);
                    break;
            }
        }
    }
    
    // Return current state to LVGL
    data->key = last_key;
    data->state = last_state;
}

/**
 * @brief Initialize LVGL input device driver for buttons
 */
esp_err_t lvgl_port_indev_init(QueueHandle_t button_queue)
{
    ESP_LOGI(TAG, "Initializing LVGL input device port");
    
    if (button_queue == NULL) {
        ESP_LOGE(TAG, "Button queue is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    btn_queue = button_queue;
    
    // Create default group for widgets
    default_group = lv_group_create();
    if (default_group == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL group");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize input device driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = button_read;
    
    // Register input device
    indev_button = lv_indev_drv_register(&indev_drv);
    if (indev_button == NULL) {
        ESP_LOGE(TAG, "Failed to register input device");
        lv_group_del(default_group);
        return ESP_FAIL;
    }
    
    // Assign the group to the input device
    lv_indev_set_group(indev_button, default_group);
    
    // Set the default group to be the active group
    lv_group_set_default(default_group);
    
    ESP_LOGI(TAG, "LVGL input device port initialized");
    return ESP_OK;
}

/**
 * @brief Get the input device object
 */
lv_indev_t *lvgl_port_indev_get(void)
{
    return indev_button;
}