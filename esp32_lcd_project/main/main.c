/**
 * ESP32-C3 Circular LCD (GC9A01) with Push Buttons Example
 * 
 * This example demonstrates how to use a 240x240 circular LCD display
 * with the GC9A01 controller and interact with it using push buttons.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "lcd_driver.h"
#include "button_handler.h"

static const char *TAG = "MAIN";

// Queue for button events
static QueueHandle_t button_queue = NULL;

// Current menu/screen state
typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_COLOR_TEST,
    SCREEN_CIRCLE_DEMO,
    SCREEN_TEXT_DEMO,
    SCREEN_INFO
} screen_state_t;

static screen_state_t current_screen = SCREEN_MAIN_MENU;
static int menu_selection = 0;
static const int menu_items_count = 4;

// Color definitions
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFC00
#define COLOR_PURPLE      0x8010

// Display dimensions
#define LCD_H_RES         240
#define LCD_V_RES         240

// Function prototypes
static void display_main_menu(void);
static void display_color_test(void);
static void display_circle_demo(void);
static void display_text_demo(void);
static void display_info(void);
static void handle_button_event(button_event_t event);

/**
 * Button event handler task
 */
static void button_task(void *pvParameters)
{
    button_event_t event;
    
    while (1) {
        if (xQueueReceive(button_queue, &event, portMAX_DELAY) == pdTRUE) {
            handle_button_event(event);
        }
    }
}

/**
 * Handle button events based on current screen
 */
static void handle_button_event(button_event_t event)
{
    ESP_LOGI(TAG, "Button event: button=%d, type=%s", 
             event.button_num, 
             event.event_type == BUTTON_PRESS ? "PRESS" : 
             event.event_type == BUTTON_RELEASE ? "RELEASE" : "LONG_PRESS");
    
    if (event.event_type != BUTTON_PRESS) {
        return; // Only handle press events for now
    }
    
    switch (current_screen) {
        case SCREEN_MAIN_MENU:
            if (event.button_num == 0) { // Navigate menu
                menu_selection = (menu_selection + 1) % menu_items_count;
                display_main_menu();
            } else if (event.button_num == 1) { // Select menu item
                switch (menu_selection) {
                    case 0:
                        current_screen = SCREEN_COLOR_TEST;
                        display_color_test();
                        break;
                    case 1:
                        current_screen = SCREEN_CIRCLE_DEMO;
                        display_circle_demo();
                        break;
                    case 2:
                        current_screen = SCREEN_TEXT_DEMO;
                        display_text_demo();
                        break;
                    case 3:
                        current_screen = SCREEN_INFO;
                        display_info();
                        break;
                }
            }
            break;
            
        default:
            // Any button returns to main menu from demo screens
            current_screen = SCREEN_MAIN_MENU;
            display_main_menu();
            break;
    }
}

/**
 * Display main menu
 */
static void display_main_menu(void)
{
    lcd_clear(COLOR_BLACK);
    
    // Draw title
    lcd_draw_string(60, 20, "MAIN MENU", COLOR_WHITE, COLOR_BLACK, 2);
    
    // Draw menu items
    const char *menu_items[] = {
        "Color Test",
        "Circle Demo",
        "Text Demo",
        "System Info"
    };
    
    for (int i = 0; i < menu_items_count; i++) {
        uint16_t color = (i == menu_selection) ? COLOR_YELLOW : COLOR_WHITE;
        uint16_t bg_color = (i == menu_selection) ? COLOR_BLUE : COLOR_BLACK;
        
        // Draw selection background
        if (i == menu_selection) {
            lcd_draw_filled_rectangle(20, 60 + i * 40, 220, 90 + i * 40, bg_color);
        }
        
        lcd_draw_string(40, 70 + i * 40, menu_items[i], color, bg_color, 1);
    }
    
    // Draw instructions
    lcd_draw_string(20, 210, "BTN1:Nav BTN2:Select", COLOR_CYAN, COLOR_BLACK, 1);
}

/**
 * Display color test pattern
 */
static void display_color_test(void)
{
    // Draw color bars
    uint16_t colors[] = {
        COLOR_RED, COLOR_GREEN, COLOR_BLUE,
        COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA,
        COLOR_WHITE, COLOR_ORANGE, COLOR_PURPLE
    };
    
    int bar_width = LCD_H_RES / 3;
    int bar_height = LCD_V_RES / 3;
    
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int x = col * bar_width;
            int y = row * bar_height;
            lcd_draw_filled_rectangle(x, y, x + bar_width - 1, y + bar_height - 1, 
                                    colors[row * 3 + col]);
        }
    }
    
    // Draw title
    lcd_draw_string(50, 110, "COLOR TEST", COLOR_BLACK, COLOR_WHITE, 2);
}

/**
 * Display circle demo
 */
static void display_circle_demo(void)
{
    lcd_clear(COLOR_BLACK);
    
    // Draw concentric circles
    for (int r = 10; r <= 110; r += 20) {
        uint16_t color = (r / 20) % 2 ? COLOR_CYAN : COLOR_MAGENTA;
        lcd_draw_circle(LCD_H_RES / 2, LCD_V_RES / 2, r, color);
    }
    
    // Draw filled circles in corners
    lcd_draw_filled_circle(30, 30, 20, COLOR_RED);
    lcd_draw_filled_circle(210, 30, 20, COLOR_GREEN);
    lcd_draw_filled_circle(30, 210, 20, COLOR_BLUE);
    lcd_draw_filled_circle(210, 210, 20, COLOR_YELLOW);
    
    // Draw title
    lcd_draw_string(60, 110, "CIRCLES", COLOR_WHITE, COLOR_BLACK, 2);
}

/**
 * Display text demo
 */
static void display_text_demo(void)
{
    lcd_clear(COLOR_BLACK);
    
    // Draw text in different sizes and colors
    lcd_draw_string(30, 20, "ESP32-C3", COLOR_RED, COLOR_BLACK, 3);
    lcd_draw_string(30, 60, "Circular LCD Demo", COLOR_GREEN, COLOR_BLACK, 2);
    lcd_draw_string(30, 90, "240x240 pixels", COLOR_BLUE, COLOR_BLACK, 1);
    lcd_draw_string(30, 110, "GC9A01 Driver", COLOR_YELLOW, COLOR_BLACK, 1);
    
    // Draw rotating text
    static int angle = 0;
    angle = (angle + 30) % 360;
    
    // Draw some special characters
    lcd_draw_string(30, 140, "Temperature: 25°C", COLOR_CYAN, COLOR_BLACK, 1);
    lcd_draw_string(30, 160, "Humidity: 45%", COLOR_MAGENTA, COLOR_BLACK, 1);
    lcd_draw_string(30, 180, "Pressure: 1013hPa", COLOR_ORANGE, COLOR_BLACK, 1);
    
    lcd_draw_string(30, 210, "Press any button...", COLOR_WHITE, COLOR_BLACK, 1);
}

/**
 * Display system information
 */
static void display_info(void)
{
    lcd_clear(COLOR_BLACK);
    
    lcd_draw_string(30, 20, "SYSTEM INFO", COLOR_YELLOW, COLOR_BLACK, 2);
    
    // Get chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    char info_str[64];
    
    snprintf(info_str, sizeof(info_str), "Chip: ESP32-C3");
    lcd_draw_string(20, 60, info_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    snprintf(info_str, sizeof(info_str), "Cores: %d", chip_info.cores);
    lcd_draw_string(20, 80, info_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    snprintf(info_str, sizeof(info_str), "Revision: %d", chip_info.revision);
    lcd_draw_string(20, 100, info_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    snprintf(info_str, sizeof(info_str), "Flash: %dMB %s", 
             spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    lcd_draw_string(20, 120, info_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    snprintf(info_str, sizeof(info_str), "Free Heap: %d KB", 
             esp_get_free_heap_size() / 1024);
    lcd_draw_string(20, 140, info_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    // Display time since boot
    int64_t uptime = esp_timer_get_time() / 1000000; // Convert to seconds
    snprintf(info_str, sizeof(info_str), "Uptime: %lld sec", uptime);
    lcd_draw_string(20, 160, info_str, COLOR_WHITE, COLOR_BLACK, 1);
    
    lcd_draw_string(20, 210, "Press any button...", COLOR_CYAN, COLOR_BLACK, 1);
}

/**
 * Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-C3 Circular LCD Demo Starting...");
    
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
    
    // Clear screen and display welcome message
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(40, 100, "ESP32-C3", COLOR_WHITE, COLOR_BLACK, 3);
    lcd_draw_string(50, 140, "LCD Demo", COLOR_CYAN, COLOR_BLACK, 2);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Display main menu
    display_main_menu();
    
    // Create button handler task
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    
    // Main loop - could be used for animations or periodic updates
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}