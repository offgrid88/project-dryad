/**
 * ESP32-C3 Circular LCD (GC9A01) with LVGL and Push Buttons
 * 
 * This example demonstrates how to use LVGL graphics library with
 * a 240x240 circular LCD display and interact with it using push buttons.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

// Hardware drivers
#include "lcd_driver.h"
#include "button_handler.h"

// LVGL
#include "lvgl.h"
#include "lvgl_port_display.h"
#include "lvgl_port_indev.h"

static const char *TAG = "MAIN";

// Task handles
static TaskHandle_t lvgl_task_handle = NULL;

// Queue for button events
static QueueHandle_t button_queue = NULL;

// Screen objects
static lv_obj_t *home_screen = NULL;
static lv_obj_t *gauge_screen = NULL;
static lv_obj_t *settings_screen = NULL;
static lv_obj_t *info_screen = NULL;

// Gauge needle for animation
static lv_meter_indicator_t *gauge_needle = NULL;

// Forward declarations
static void create_home_screen(void);
static void create_gauge_screen(void);
static void create_settings_screen(void);
static void create_info_screen(void);
static void lvgl_task(void *pvParameters);

// Event callbacks
static void back_btn_event_cb(lv_event_t *e);
static void gauge_menu_event_cb(lv_event_t *e);
static void settings_menu_event_cb(lv_event_t *e);
static void info_menu_event_cb(lv_event_t *e);
static void brightness_slider_event_cb(lv_event_t *e);
static void gauge_anim_exec_cb(void *var, int32_t v);

/**
 * Back button event callback
 */
static void back_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load(home_screen);
    }
}

/**
 * Gauge menu button event callback
 */
static void gauge_menu_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        create_gauge_screen();
        lv_scr_load(gauge_screen);
    }
}

/**
 * Settings menu button event callback
 */
static void settings_menu_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        create_settings_screen();
        lv_scr_load(settings_screen);
    }
}

/**
 * Info menu button event callback
 */
static void info_menu_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        create_info_screen();
        lv_scr_load(info_screen);
    }
}

/**
 * Brightness slider event callback
 */
static void brightness_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    lcd_set_brightness((uint8_t)value);
}

/**
 * Gauge animation callback
 */
static void gauge_anim_exec_cb(void *var, int32_t v)
{
    lv_meter_set_indicator_value((lv_obj_t *)var, gauge_needle, v);
}

/**
 * Create home screen with menu
 */
static void create_home_screen(void)
{
    home_screen = lv_obj_create(NULL);
    
    // Create a label for the title
    lv_obj_t *title = lv_label_create(home_screen);
    lv_label_set_text(title, "ESP32-C3 LCD");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    
    // Create a list for menu items
    lv_obj_t *list = lv_list_create(home_screen);
    lv_obj_set_size(list, 200, 140);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 10);
    
    // Add menu items
    lv_obj_t *btn;
    
    btn = lv_list_add_btn(list, LV_SYMBOL_GAUGE, "Gauges Demo");
    lv_obj_add_event_cb(btn, gauge_menu_event_cb, LV_EVENT_CLICKED, NULL);
    
    btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Settings");
    lv_obj_add_event_cb(btn, settings_menu_event_cb, LV_EVENT_CLICKED, NULL);
    
    btn = lv_list_add_btn(list, LV_SYMBOL_INFO, "System Info");
    lv_obj_add_event_cb(btn, info_menu_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Add navigation hint
    lv_obj_t *hint = lv_label_create(home_screen);
    lv_label_set_text(hint, "BTN1: Nav  BTN2: Select");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
}

/**
 * Create gauge demo screen
 */
static void create_gauge_screen(void)
{
    if (gauge_screen != NULL) {
        return; // Already created
    }
    
    gauge_screen = lv_obj_create(NULL);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(gauge_screen);
    lv_obj_set_size(back_btn, 80, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Create a circular meter/gauge
    lv_obj_t *meter = lv_meter_create(gauge_screen);
    lv_obj_set_size(meter, 180, 180);
    lv_obj_center(meter);
    
    // Add scale
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 8, 4, 15, lv_color_black(), 10);
    lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135);
    
    // Add indicators
    lv_meter_indicator_t *indic;
    
    // Add a blue arc indicator
    indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 20);
    
    // Add a red arc indicator
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 80);
    lv_meter_set_indicator_end_value(meter, indic, 100);
    
    // Add a needle
    gauge_needle = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_ORANGE), -10);
    
    // Create animation for the needle
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, gauge_anim_exec_cb);
    lv_anim_set_var(&a, meter);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_time(&a, 3000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_playback_time(&a, 3000);
    lv_anim_start(&a);
    
    // Add title
    lv_obj_t *title = lv_label_create(gauge_screen);
    lv_label_set_text(title, "Meter Demo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
}

/**
 * Create settings screen
 */
static void create_settings_screen(void)
{
    if (settings_screen != NULL) {
        return; // Already created
    }
    
    settings_screen = lv_obj_create(NULL);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(back_btn, 80, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Title
    lv_obj_t *title = lv_label_create(settings_screen);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    
    // Create a container for settings
    lv_obj_t *cont = lv_obj_create(settings_screen);
    lv_obj_set_size(cont, 220, 150);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont, 10, 0);
    lv_obj_set_style_pad_gap(cont, 10, 0);
    
    // Brightness slider
    lv_obj_t *bright_label = lv_label_create(cont);
    lv_label_set_text(bright_label, "Brightness");
    
    lv_obj_t *slider = lv_slider_create(cont);
    lv_obj_set_width(slider, 180);
    lv_slider_set_value(slider, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // LED switch
    lv_obj_t *led_label = lv_label_create(cont);
    lv_label_set_text(led_label, "Status LED");
    
    lv_obj_t *sw = lv_switch_create(cont);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    
    // Theme selector
    lv_obj_t *theme_label = lv_label_create(cont);
    lv_label_set_text(theme_label, "Dark Theme");
    
    lv_obj_t *theme_sw = lv_switch_create(cont);
    lv_obj_add_state(theme_sw, LV_STATE_CHECKED);
}

/**
 * Create system info screen
 */
static void create_info_screen(void)
{
    if (info_screen != NULL) {
        return; // Already created
    }
    
    info_screen = lv_obj_create(NULL);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(info_screen);
    lv_obj_set_size(back_btn, 80, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Title
    lv_obj_t *title = lv_label_create(info_screen);
    lv_label_set_text(title, "System Info");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    
    // Info container
    lv_obj_t *info_cont = lv_obj_create(info_screen);
    lv_obj_set_size(info_cont, 220, 150);
    lv_obj_align(info_cont, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_pad_all(info_cont, 10, 0);
    
    // Get chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    char info_text[256];
    snprintf(info_text, sizeof(info_text),
        "Chip: ESP32-C3\n"
        "Cores: %d\n"
        "Flash: %dMB\n"
        "Free Heap: %d KB\n"
        "LVGL Version: %d.%d.%d\n"
        "Uptime: %lld sec",
        chip_info.cores,
        spi_flash_get_chip_size() / (1024 * 1024),
        esp_get_free_heap_size() / 1024,
        LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
        esp_timer_get_time() / 1000000
    );
    
    lv_obj_t *info_label = lv_label_create(info_cont);
    lv_label_set_text(info_label, info_text);
    lv_obj_center(info_label);
}

/**
 * LVGL task - handles LVGL timer
 */
static void lvgl_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    
    while (1) {
        // Lock LVGL mutex
        if (lvgl_port_lock(-1)) {
            lv_task_handler();
            lvgl_port_unlock();
        }
        
        // Run every 10ms
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-C3 LVGL Demo Starting...");
    
    // Create button event queue
    button_queue = xQueueCreate(10, sizeof(button_event_t));
    if (button_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return;
    }
    
    // Initialize LCD
    ESP_LOGI(TAG, "Initializing LCD...");
    esp_err_t ret = lcd_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD");
        return;
    }
    
    // Initialize buttons
    ESP_LOGI(TAG, "Initializing buttons...");
    ret = button_init(button_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize buttons");
        return;
    }
    
    // Initialize LVGL display port
    ESP_LOGI(TAG, "Initializing LVGL display...");
    ret = lvgl_port_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL display");
        return;
    }
    
    // Initialize LVGL input device port
    ESP_LOGI(TAG, "Initializing LVGL input...");
    ret = lvgl_port_indev_init(button_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL input");
        return;
    }
    
    // Create LVGL task
    xTaskCreate(lvgl_task, "lvgl_task", 4096, NULL, 10, &lvgl_task_handle);
    
    // Wait a bit for LVGL to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Create and load home screen
    if (lvgl_port_lock(-1)) {
        // Apply dark theme
        lv_theme_t *theme = lv_theme_default_init(
            lv_disp_get_default(),
            lv_palette_main(LV_PALETTE_BLUE),
            lv_palette_main(LV_PALETTE_RED),
            true,  // Dark mode
            &lv_font_montserrat_14
        );
        lv_disp_set_theme(lv_disp_get_default(), theme);
        
        create_home_screen();
        lv_scr_load(home_screen);
        
        lvgl_port_unlock();
    }
    
    ESP_LOGI(TAG, "LVGL Demo ready!");
    
    // Main loop - just keep the task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}