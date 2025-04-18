// Display app
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "display";

void display_task(void* pvParameters) {
  // TODO: setup i2c

  while (true) {
    // TODO: test display

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}