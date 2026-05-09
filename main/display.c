#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "lcd_driver.h"
#include "button_handler.h"
#include "lvgl_port_display.h"
#include "lvgl_port_indev.h"
#include "lvgl.h"

static const char *TAG = "display";

// Shared moisture data (written by moisture task, read by display task)
static volatile int s_moisture_percent = 0;
static SemaphoreHandle_t s_moisture_mutex = NULL;

// LVGL UI objects
static lv_obj_t *s_moisture_label = NULL;
static lv_obj_t *s_status_label   = NULL;
static lv_meter_indicator_t *s_gauge_needle = NULL;
static lv_obj_t *s_gauge_meter    = NULL;

static QueueHandle_t s_button_queue = NULL;

// ── Shared data API ──────────────────────────────────────────────────────────

void display_set_moisture(int percent)
{
    if (s_moisture_mutex && xSemaphoreTake(s_moisture_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_moisture_percent = percent;
        xSemaphoreGive(s_moisture_mutex);
    }
}

static int get_moisture(void)
{
    int val = 0;
    if (s_moisture_mutex && xSemaphoreTake(s_moisture_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        val = s_moisture_percent;
        xSemaphoreGive(s_moisture_mutex);
    }
    return val;
}

// ── UI helpers ───────────────────────────────────────────────────────────────

static const char *moisture_status(int pct)
{
    if (pct < 20) return "Thirsty!";
    if (pct < 50) return "Doing OK";
    if (pct < 80) return "Happy :)";
    return "Too wet!";
}

static lv_color_t moisture_color(int pct)
{
    if (pct < 20) return lv_palette_main(LV_PALETTE_RED);
    if (pct < 50) return lv_palette_main(LV_PALETTE_ORANGE);
    if (pct < 80) return lv_palette_main(LV_PALETTE_GREEN);
    return lv_palette_main(LV_PALETTE_BLUE);
}

// ── Screen creation ──────────────────────────────────────────────────────────

static void create_plant_screen(lv_obj_t *screen)
{
    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Dryad");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    // Moisture gauge
    s_gauge_meter = lv_meter_create(screen);
    lv_obj_set_size(s_gauge_meter, 160, 160);
    lv_obj_align(s_gauge_meter, LV_ALIGN_CENTER, 0, 0);

    lv_meter_scale_t *scale = lv_meter_add_scale(s_gauge_meter);
    lv_meter_set_scale_ticks(s_gauge_meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(s_gauge_meter, scale, 8, 4, 15, lv_color_black(), 10);
    lv_meter_set_scale_range(s_gauge_meter, scale, 0, 100, 270, 135);

    // Dry zone (red)
    lv_meter_indicator_t *indic;
    indic = lv_meter_add_scale_lines(s_gauge_meter, scale,
        lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
    lv_meter_set_indicator_start_value(s_gauge_meter, indic, 0);
    lv_meter_set_indicator_end_value(s_gauge_meter, indic, 20);

    // Good zone (green arc)
    indic = lv_meter_add_arc(s_gauge_meter, scale, 3, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(s_gauge_meter, indic, 20);
    lv_meter_set_indicator_end_value(s_gauge_meter, indic, 80);

    // Wet zone (blue)
    indic = lv_meter_add_scale_lines(s_gauge_meter, scale,
        lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE), false, 0);
    lv_meter_set_indicator_start_value(s_gauge_meter, indic, 80);
    lv_meter_set_indicator_end_value(s_gauge_meter, indic, 100);

    // Needle
    s_gauge_needle = lv_meter_add_needle_line(s_gauge_meter, scale, 4,
        lv_palette_main(LV_PALETTE_ORANGE), -10);

    // Moisture % label
    s_moisture_label = lv_label_create(screen);
    lv_label_set_text(s_moisture_label, "-- %");
    lv_obj_align(s_moisture_label, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_text_font(s_moisture_label, &lv_font_montserrat_16, 0);

    // Status label
    s_status_label = lv_label_create(screen);
    lv_label_set_text(s_status_label, "Reading...");
    lv_obj_align(s_status_label, LV_ALIGN_BOTTOM_MID, 0, -12);
}

// ── LVGL update (called every second from LVGL task) ─────────────────────────

static void refresh_ui(void)
{
    int pct = get_moisture();

    if (s_gauge_meter && s_gauge_needle)
        lv_meter_set_indicator_value(s_gauge_meter, s_gauge_needle, pct);

    if (s_moisture_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d %%", pct);
        lv_label_set_text(s_moisture_label, buf);
        lv_obj_set_style_text_color(s_moisture_label, moisture_color(pct), 0);
    }

    if (s_status_label)
        lv_label_set_text(s_status_label, moisture_status(pct));
}

// ── LVGL task ────────────────────────────────────────────────────────────────

static void lvgl_task(void *pvParameters)
{
    uint32_t refresh_tick = 0;

    while (1) {
        if (lvgl_port_lock(pdMS_TO_TICKS(10))) {
            lv_task_handler();

            // Refresh plant data every second
            uint32_t now = xTaskGetTickCount();
            if ((now - refresh_tick) >= pdMS_TO_TICKS(1000)) {
                refresh_ui();
                refresh_tick = now;
            }

            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ── Display task (entry point from main) ─────────────────────────────────────

void display_task(void *pvParameters)
{
    s_moisture_mutex = xSemaphoreCreateMutex();
    if (!s_moisture_mutex) {
        ESP_LOGE(TAG, "Failed to create moisture mutex");
        vTaskDelete(NULL);
        return;
    }

    s_button_queue = xQueueCreate(10, sizeof(button_event_t));
    if (!s_button_queue) {
        ESP_LOGE(TAG, "Failed to create button queue");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Initializing LCD");
    if (lcd_init() != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Initializing buttons");
    if (button_init(s_button_queue) != ESP_OK) {
        ESP_LOGE(TAG, "Button init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Initializing LVGL display port");
    if (lvgl_port_display_init() != ESP_OK) {
        ESP_LOGE(TAG, "LVGL display init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Initializing LVGL input port");
    if (lvgl_port_indev_init(s_button_queue) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL input init failed");
        vTaskDelete(NULL);
        return;
    }

    // Start LVGL task with adequate stack (LVGL 8.x needs >=16KB)
    TaskHandle_t lvgl_handle = NULL;
    if (xTaskCreate(lvgl_task, "lvgl", 16384, NULL, 5, &lvgl_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        vTaskDelete(NULL);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    if (lvgl_port_lock(portMAX_DELAY)) {
        lv_theme_t *theme = lv_theme_default_init(
            lv_disp_get_default(),
            lv_palette_main(LV_PALETTE_GREEN),
            lv_palette_main(LV_PALETTE_ORANGE),
            true,
            &lv_font_montserrat_14
        );
        lv_disp_set_theme(lv_disp_get_default(), theme);

        lv_obj_t *screen = lv_obj_create(NULL);
        create_plant_screen(screen);
        lv_scr_load(screen);

        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "Display ready");
    vTaskDelete(NULL);
}
