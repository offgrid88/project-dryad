#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "moisture_sensor.h"
#include "display.h"

// Task configuration
#define MOISTURE_TASK_STACK_SIZE  4096
#define MOISTURE_TASK_PRIORITY    1
#define MOISTURE_TASK_CORE        0

#define DISPLAY_TASK_STACK_SIZE   4096
#define DISPLAY_TASK_PRIORITY     1
#define DISPLAY_TASK_CORE         0

// Task handles
static TaskHandle_t moisture_task_handle = NULL;
static TaskHandle_t display_task_handle = NULL;

static const char *TAG = "Main";

void app_main(void) {
    // Create the moisture sensor task
    xTaskCreatePinnedToCore(
        moisture_sensor_task,    // Task function
        "moistureSensor",        // Task name
        MOISTURE_TASK_STACK_SIZE,// Stack size
        NULL,                    // Parameters
        MOISTURE_TASK_PRIORITY,  // Priority
        &moisture_task_handle,   // Task handle
        MOISTURE_TASK_CORE       // Core ID
    );

    // Create the display task
    xTaskCreatePinnedToCore(
        display_task,            // Task function
        "display",               // Task name
        DISPLAY_TASK_STACK_SIZE, // Stack size
        NULL,                    // Parameters
        DISPLAY_TASK_PRIORITY,   // Priority
        &display_task_handle,    // Task handle
        DISPLAY_TASK_CORE        // Core ID
    );

    // Add any other initialization or tasks here
    ESP_LOGI(TAG, "Application started");
}