/**
 * @file lvgl_port_display.c
 * @brief Minimal LVGL display port — synchronous semaphore flush, no separate task.
 *
 * disp_flush starts DMA, then blocks on s_flush_done until the on_color_trans_done
 * ISR fires, then calls lv_disp_flush_ready.  This serialises render+flush per tile
 * but removes all async race conditions for debugging.
 */

#include "lvgl_port_display.h"
#include "lcd_driver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "LVGL_PORT";

#define LCD_H_RES       240
#define LCD_V_RES       320
#define DISP_BUF_LINES  32
#define DISP_BUF_SIZE   (LCD_H_RES * DISP_BUF_LINES)

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t      disp_drv;
static lv_color_t        *buf1;
static SemaphoreHandle_t  s_flush_done;

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static bool on_trans_done(esp_lcd_panel_io_handle_t io,
                          esp_lcd_panel_io_event_data_t *edata,
                          void *user_ctx)
{
    (void)io; (void)edata; (void)user_ctx;
    BaseType_t woke = pdFALSE;
    xSemaphoreGiveFromISR(s_flush_done, &woke);
    return woke == pdTRUE;
}

static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    lcd_draw_bitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    xSemaphoreTake(s_flush_done, portMAX_DELAY);
    lv_disp_flush_ready(drv);
}

esp_err_t lvgl_port_display_init(void)
{
    ESP_LOGI(TAG, "LVGL init (sync flush, single buf, 240x320)");

    s_flush_done = xSemaphoreCreateBinary();
    if (!s_flush_done) {
        ESP_LOGE(TAG, "Semaphore alloc failed");
        return ESP_ERR_NO_MEM;
    }

    lv_init();

    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name     = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000));

    buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    if (!buf1) {
        ESP_LOGE(TAG, "Buffer alloc failed");
        return ESP_ERR_NO_MEM;
    }

    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, DISP_BUF_SIZE);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = LCD_H_RES;
    disp_drv.ver_res  = LCD_V_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    esp_lcd_panel_io_handle_t io = lcd_get_io_handle();
    if (!io) {
        ESP_LOGE(TAG, "No LCD IO handle");
        return ESP_FAIL;
    }
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = on_trans_done,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io, &cbs, NULL));

    /* Drain any stale gives from i80 bus activity before LVGL starts flushing */
    xSemaphoreTake(s_flush_done, 0);

    ESP_LOGI(TAG, "LVGL display ready");
    return ESP_OK;
}

lv_disp_t *lvgl_port_display_get(void) { return NULL; }

/* No separate LVGL task — single-threaded access, no mutex needed */
bool lvgl_port_lock(int timeout_ms)  { (void)timeout_ms; return true; }
void lvgl_port_unlock(void)          {}
