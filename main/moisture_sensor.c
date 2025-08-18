#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "moisture_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "moisture_sensor";

// TODO: these values need calibration
#define DRY_VALUE 1200
#define WET_VALUE 3600

// For ESP32-C6, ADC1 channels are available on specific GPIOs
#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_0   // GPIO0 for ESP32-C6
#define ADC_ATTEN       ADC_ATTEN_DB_11  // 11dB attenuation (0-3.3V range)

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

    // Try to calibrate ADC using curve fitting for ESP32-C6
    // ESP-IDF 5.5 uses curve fitting calibration scheme
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        do_calibration = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    } else {
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
        read_moisture_percent();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}