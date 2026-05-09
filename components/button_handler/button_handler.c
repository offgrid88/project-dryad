/**
 * @file button_handler.c
 * @brief Button handler implementation for ESP32-C3
 */

#include <string.h>
#include "button_handler.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static const char *TAG = "BUTTON_HANDLER";

#define MAX_BUTTONS 8
#define TASK_STACK_SIZE 2048
#define TASK_PRIORITY 10

// Button state structure
typedef struct {
    button_config_t config;
    bool is_configured;
    bool current_state;
    bool last_state;
    uint32_t state_change_time;
    uint32_t press_time;
    bool long_press_sent;
} button_state_t;

// Module state
static struct {
    button_state_t buttons[MAX_BUTTONS];
    QueueHandle_t event_queue;
    TaskHandle_t task_handle;
    bool is_initialized;
} button_handler = {0};

/**
 * Get current time in milliseconds
 */
static uint32_t get_time_ms(void)
{
    return esp_timer_get_time() / 1000;
}

/**
 * Read button state (considering active_low configuration)
 */
static bool read_button_state(uint8_t button_num)
{
    if (!button_handler.buttons[button_num].is_configured) {
        return false;
    }
    
    int level = gpio_get_level(button_handler.buttons[button_num].config.gpio_num);
    
    // Return true if button is pressed
    if (button_handler.buttons[button_num].config.active_low) {
        return level == 0;
    } else {
        return level == 1;
    }
}

/**
 * Send button event to queue
 */
static void send_event(uint8_t button_num, button_event_type_t event_type)
{
    if (button_handler.event_queue == NULL) {
        return;
    }
    
    button_event_t event = {
        .button_num = button_num,
        .event_type = event_type,
        .timestamp = get_time_ms()
    };
    
    if (xQueueSend(button_handler.event_queue, &event, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to send button event");
    }
}

/**
 * Button handler task
 */
static void button_task(void *pvParameters)
{
    const TickType_t task_period = pdMS_TO_TICKS(10); // 10ms scan rate
    
    while (button_handler.is_initialized) {
        uint32_t current_time = get_time_ms();
        
        // Scan all buttons
        for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
            if (!button_handler.buttons[i].is_configured) {
                continue;
            }
            
            button_state_t *btn = &button_handler.buttons[i];
            bool raw_state = read_button_state(i);
            
            // Check if state has changed
            if (raw_state != btn->last_state) {
                btn->state_change_time = current_time;
                btn->last_state = raw_state;
            }
            
            // Check if debounce time has passed
            if ((current_time - btn->state_change_time) >= btn->config.debounce_ms) {
                bool debounced_state = btn->last_state;
                
                // State has changed after debounce
                if (debounced_state != btn->current_state) {
                    btn->current_state = debounced_state;
                    
                    if (debounced_state) {
                        // Button pressed
                        btn->press_time = current_time;
                        btn->long_press_sent = false;
                        send_event(i, BUTTON_PRESS);
                        ESP_LOGD(TAG, "Button %d pressed", i);
                    } else {
                        // Button released
                        send_event(i, BUTTON_RELEASE);
                        ESP_LOGD(TAG, "Button %d released", i);
                    }
                }
                
                // Check for long press
                if (btn->current_state && !btn->long_press_sent) {
                    if ((current_time - btn->press_time) >= btn->config.long_press_ms) {
                        btn->long_press_sent = true;
                        send_event(i, BUTTON_LONG_PRESS);
                        ESP_LOGD(TAG, "Button %d long press", i);
                    }
                }
            }
        }
        
        vTaskDelay(task_period);
    }
    
    vTaskDelete(NULL);
}

/**
 * Initialize button handler
 */
esp_err_t button_init(QueueHandle_t event_queue)
{
    if (button_handler.is_initialized) {
        ESP_LOGW(TAG, "Button handler already initialized");
        return ESP_OK;
    }
    
    if (event_queue == NULL) {
        ESP_LOGE(TAG, "Event queue is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Clear state
    memset(&button_handler, 0, sizeof(button_handler));
    
    button_handler.event_queue = event_queue;
    button_handler.is_initialized = true;
    
    // Create handler task
    BaseType_t ret = xTaskCreate(button_task, "button_task", 
                                  TASK_STACK_SIZE, NULL, 
                                  TASK_PRIORITY, &button_handler.task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        button_handler.is_initialized = false;
        return ESP_FAIL;
    }
    
    // Configure default buttons (2 buttons on GPIO 0 and 2)
    button_config_t default_config = {
        .gpio_num = 0,
        .active_low = true,
        .pull_up_en = true,
        .pull_down_en = false,
        .debounce_ms = 50,
        .long_press_ms = 1000
    };
    
    // Button 0 on GPIO 0
    button_add(0, &default_config);
    
    // Button 1 on GPIO 2
    default_config.gpio_num = 2;
    button_add(1, &default_config);
    
    ESP_LOGI(TAG, "Button handler initialized");
    return ESP_OK;
}

/**
 * Add a button to the handler
 */
esp_err_t button_add(uint8_t button_num, const button_config_t *config)
{
    if (!button_handler.is_initialized) {
        ESP_LOGE(TAG, "Button handler not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (button_num >= MAX_BUTTONS) {
        ESP_LOGE(TAG, "Button number %d exceeds maximum %d", button_num, MAX_BUTTONS);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Button config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = config->pull_up_en ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = config->pull_down_en ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d", config->gpio_num);
        return ret;
    }
    
    // Store button configuration
    memcpy(&button_handler.buttons[button_num].config, config, sizeof(button_config_t));
    button_handler.buttons[button_num].is_configured = true;
    button_handler.buttons[button_num].current_state = false;
    button_handler.buttons[button_num].last_state = read_button_state(button_num);
    button_handler.buttons[button_num].state_change_time = get_time_ms();
    button_handler.buttons[button_num].long_press_sent = false;
    
    ESP_LOGI(TAG, "Added button %d on GPIO %d", button_num, config->gpio_num);
    return ESP_OK;
}

/**
 * Remove a button from the handler
 */
esp_err_t button_remove(uint8_t button_num)
{
    if (!button_handler.is_initialized) {
        ESP_LOGE(TAG, "Button handler not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (button_num >= MAX_BUTTONS) {
        ESP_LOGE(TAG, "Button number %d exceeds maximum %d", button_num, MAX_BUTTONS);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!button_handler.buttons[button_num].is_configured) {
        ESP_LOGW(TAG, "Button %d not configured", button_num);
        return ESP_OK;
    }
    
    // Reset GPIO to default state
    gpio_reset_pin(button_handler.buttons[button_num].config.gpio_num);
    
    // Clear button state
    memset(&button_handler.buttons[button_num], 0, sizeof(button_state_t));
    
    ESP_LOGI(TAG, "Removed button %d", button_num);
    return ESP_OK;
}

/**
 * Get current state of a button
 */
bool button_is_pressed(uint8_t button_num)
{
    if (!button_handler.is_initialized || button_num >= MAX_BUTTONS) {
        return false;
    }
    
    if (!button_handler.buttons[button_num].is_configured) {
        return false;
    }
    
    return button_handler.buttons[button_num].current_state;
}

/**
 * Deinitialize button handler
 */
esp_err_t button_deinit(void)
{
    if (!button_handler.is_initialized) {
        ESP_LOGW(TAG, "Button handler not initialized");
        return ESP_OK;
    }
    
    button_handler.is_initialized = false;
    
    // Wait for task to finish
    if (button_handler.task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Remove all buttons
    for (uint8_t i = 0; i < MAX_BUTTONS; i++) {
        if (button_handler.buttons[i].is_configured) {
            button_remove(i);
        }
    }
    
    // Clear state
    memset(&button_handler, 0, sizeof(button_handler));
    
    ESP_LOGI(TAG, "Button handler deinitialized");
    return ESP_OK;
}