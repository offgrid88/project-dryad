#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_driver.h"
#include "lvgl_port_display.h"
#include "lvgl.h"

#include <stdio.h>

static const char *TAG = "display";

static volatile int s_moisture = -1;
static volatile int s_pump_on;
static lv_obj_t *s_status;

void display_set_status(int moisture_percent, bool pump_on)
{
    s_moisture = moisture_percent;
    s_pump_on = pump_on ? 1 : 0;
}

/* ── Helpers ────────────────────────────────────────────── */

static lv_obj_t *make_line(lv_obj_t *parent,
                            lv_point_t *pts, uint16_t n,
                            uint32_t color, int width)
{
    lv_obj_t *l = lv_line_create(parent);
    lv_line_set_points(l, pts, n);
    lv_obj_set_style_line_color(l, lv_color_hex(color), 0);
    lv_obj_set_style_line_width(l, width, 0);
    lv_obj_set_style_line_rounded(l, true, 0);
    return l;
}

static lv_obj_t *make_rect(lv_obj_t *parent,
                            int w, int h, int radius,
                            uint32_t color, lv_opa_t opa,
                            int align_x, int align_y)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_align(o, LV_ALIGN_CENTER, align_x, align_y);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_shadow_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

/* ── Face ───────────────────────────────────────────────── */

static void create_face(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* Deep black background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0E0E0E), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Left eye ">"
     *    Two thick white lines meeting at a tip on the right.
     *    Screen coords: all absolute (top-left = 0,0).
     *    Eye center ~(82, 148), tip at (108, 148).
     */
    static lv_point_t lt[] = {{48, 115}, {108, 148}};
    static lv_point_t lb[] = {{108, 148}, {48, 181}};
    make_line(scr, lt, 2, 0xFFFFFF, 10);
    make_line(scr, lb, 2, 0xFFFFFF, 10);

    /* ── Right eye "<"
     *    Mirror: tip on the left at (132, 148).
     */
    static lv_point_t rt[] = {{192, 115}, {132, 148}};
    static lv_point_t rb[] = {{132, 148}, {192, 181}};
    make_line(scr, rt, 2, 0xFFFFFF, 10);
    make_line(scr, rb, 2, 0xFFFFFF, 10);

    /* ── Nose: small pink rounded oval ── */
    make_rect(scr, 22, 28, 8, 0xFF3355, LV_OPA_COVER, 0, 52);

    /* ── Mouth: tiny dark-grey rounded rect ── */
    make_rect(scr, 28, 18, 6, 0x4A4A4A, LV_OPA_COVER, 0, 86);

    s_status = lv_label_create(scr);
    lv_label_set_text(s_status, "--%");
    lv_obj_set_style_text_color(s_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -12);
}

/* ── Display task ───────────────────────────────────────── */

void display_task(void *pvParameters)
{
    (void)pvParameters;

    if (lcd_init() != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed");
        vTaskDelete(NULL);
        return;
    }
    if (lvgl_port_display_init() != ESP_OK) {
        ESP_LOGE(TAG, "LVGL init failed");
        vTaskDelete(NULL);
        return;
    }

    create_face();
    ESP_LOGI(TAG, "Face drawn");

    int shown_moisture = -2;
    int shown_pump = -1;
    while (1) {
        int moisture = s_moisture;
        int pump_on = s_pump_on;
        if (s_status && (moisture != shown_moisture || pump_on != shown_pump)) {
            char text[24];
            if (moisture < 0) {
                snprintf(text, sizeof(text), "--%%");
            } else {
                snprintf(text, sizeof(text), "%d%%  %s", moisture, pump_on ? "PUMP" : "idle");
            }
            lv_label_set_text(s_status, text);
            shown_moisture = moisture;
            shown_pump = pump_on;
        }
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
