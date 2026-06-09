#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "moisture_sensor.h"
#include "display.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "moisture_sensor";

// TODO: these values need calibration
#define DRY_VALUE 1200
#define WET_VALUE 3600

#define ADC_UNIT    ADC_UNIT_1
#define ADC_ATTEN   ADC_ATTEN_DB_12

#if CONFIG_IDF_TARGET_ESP32C6
#define ADC_CHANNEL ADC_CHANNEL_0   // GPIO0
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_CHANNEL ADC_CHANNEL_0   // GPIO0
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_CHANNEL ADC_CHANNEL_3   // GPIO4 (GPIO3 = LCD reset, cannot use)
#elif CONFIG_IDF_TARGET_ESP32
#define ADC_CHANNEL ADC_CHANNEL_6   // GPIO34 (input-only, ADC1_CH6)
#else
#error "Unsupported target: add ADC channel mapping for your board in moisture_sensor.c"
#endif

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool do_calibration = false;

void setup_adc(void) {
    // Initialize ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Configure ADC
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

#if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        do_calibration = true;
        ESP_LOGI(TAG, "ADC calibration enabled (curve fitting)");
    }
#elif CONFIG_IDF_TARGET_ESP32
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        do_calibration = true;
        ESP_LOGI(TAG, "ADC calibration enabled (line fitting)");
    }
#endif
    if (!do_calibration) {
        ESP_LOGW(TAG, "ADC calibration failed, using raw values");
    }
}

int read_moisture_percent(void) {
    // Read raw ADC value
    int raw_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw_value));

    // Convert to voltage if calibration is available
    int voltage = 0;
    if (do_calibration) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw_value, &voltage));
    } else {
        // Approximate conversion if no calibration
        voltage = (raw_value * 3300) / 4095;
    }

    // Calculate moisture percentage (inverted scale)
    int moisture_percent = 100 * (voltage - DRY_VALUE) / (WET_VALUE - DRY_VALUE);

    // Clamp values between 0-100%
    moisture_percent = (moisture_percent > 100) ? 100 : 
                      (moisture_percent < 0) ? 0 : moisture_percent;

    ESP_LOGI(TAG, "Raw: %d, Voltage: %dmV, Moisture: %d%%", raw_value, voltage, moisture_percent);

    return moisture_percent;
}

void moisture_sensor_task(void* pvParameters) {
    setup_adc();
    ESP_LOGI(TAG, "Moisture sensor initialized");

    while (1) {
        int pct = read_moisture_percent();
        display_set_moisture(pct);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}