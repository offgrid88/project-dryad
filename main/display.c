#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display.h"

static const char *TAG = "display";

void display_task(void* pvParameters) {
    ESP_LOGI(TAG, "Display task started");
    // TODO: setup i2c

    while (1) {
        // TODO: test display

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}