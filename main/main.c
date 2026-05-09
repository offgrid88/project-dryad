#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "moisture_sensor.h"
#include "display.h"

static const char *TAG = "Main";

void app_main(void)
{
    ESP_LOGI(TAG, "Dryad starting");

    BaseType_t ret;

    ret = xTaskCreate(moisture_sensor_task, "moisture", 8192, NULL, 2, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create moisture task");
        return;
    }

    // Display task self-deletes after init; LVGL task is spawned inside it
    ret = xTaskCreate(display_task, "display", 8192, NULL, 3, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create display task");
        return;
    }
}
