#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "mqtt_client.h"
#include "esp_log.h"

extern "C" void app_main();

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;  // GPIO34 if ADC1
static const adc_atten_t atten = ADC_ATTEN_DB_12;
static const adc_unit_t unit = ADC_UNIT_1;

static const char *MQTT_URI = "mqtt://mqtt.aymenrachdi.xyz";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static const char *TAG = "MQTT_APP";

// Define the dry (0% moisture) and wet (100% moisture) calibration values
const uint32_t DRY_VALUE = 0;    // adjust this value
const uint32_t WET_VALUE = 3000; // adjust this value

void mqtt_app_start(void) {
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.broker.address.uri = MQTT_URI;

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (mqtt_client == NULL) {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return;
  }

  esp_err_t err = esp_mqtt_client_start(mqtt_client);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGI(TAG, "MQTT client started successfully");
}

void send_mqtt_message(uint32_t moisture_percent) {
  char payload[100];
  sprintf(payload, "Moisture Level: %ld%%", moisture_percent);
  esp_mqtt_client_publish(mqtt_client, "soil/moisture", payload, 0, 1, 0);
}

void app_main() {
  // ADC configuration
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten((adc1_channel_t)channel, atten);
  adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
  mqtt_app_start();

  while (true) {
    uint32_t adc_reading = 0;
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
      adc_reading += adc1_get_raw((adc1_channel_t)channel);
    }
    adc_reading /= NO_OF_SAMPLES;
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

    // Calculate moisture as a percentage
    uint32_t moisture_percent = 100 * (voltage - DRY_VALUE) / (WET_VALUE - DRY_VALUE);
    moisture_percent = (moisture_percent > 100) ? 100 : (moisture_percent < 0) ? 0 : moisture_percent;

    printf("Voltage: %ldmV, Moisture Level: %ld%%\n", voltage, moisture_percent);
    send_mqtt_message(moisture_percent);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
