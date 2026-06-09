#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_driver.h"
#include "lvgl_port_display.h"
#include "lvgl.h"

static const char *TAG = "display";

void display_set_moisture(int percent) { (void)percent; }

/* ──────────────────────────────────────────────────────────
 * Robot-face helpers
 * ────────────────────────────────────────────────────────── */

static lv_obj_t *make_circle(lv_obj_t *parent, int w, int h,
                              lv_align_t align, lv_obj_t *ref,
                              int ox, int oy, uint32_t hex)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, w, h);
    if (ref)
        lv_obj_align_to(obj, ref, align, ox, oy);
    else
        lv_obj_align(obj, align, ox, oy);
    lv_obj_set_style_radius(obj,       LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj,     lv_color_hex(hex),  0);
    lv_obj_set_style_bg_opa(obj,       LV_OPA_COVER,       0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj,      0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static void create_robot_face(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Eyes (white circles) ── */
    lv_obj_t *leye = make_circle(scr, 80, 80,
                                 LV_ALIGN_CENTER, NULL, -55, -50, 0xFFFFFF);
    lv_obj_t *reye = make_circle(scr, 80, 80,
                                 LV_ALIGN_CENTER, NULL,  55, -50, 0xFFFFFF);

    /* ── Pupils (dark, offset slightly right+down) ── */
    make_circle(scr, 30, 30, LV_ALIGN_CENTER, leye, 7, 7, 0x111111);
    make_circle(scr, 30, 30, LV_ALIGN_CENTER, reye, 7, 7, 0x111111);

    /* ── Nose ── */
    make_circle(scr, 12, 12, LV_ALIGN_CENTER, NULL, 0, 10, 0x888888);

    /* ── Smile mouth (arc, bottom of circle) ──
     * LVGL 8: 0° = 3 o'clock, angles increase clockwise.
     * 45°→135° sweeps from lower-right through 90° (bottom) to lower-left.
     */
    lv_obj_t *mouth = lv_arc_create(scr);
    lv_obj_set_size(mouth, 130, 130);
    lv_obj_align(mouth, LV_ALIGN_CENTER, 0, 70);
    lv_arc_set_bg_angles(mouth, 0, 360);
    lv_arc_set_angles(mouth, 45, 135);
    lv_obj_set_style_arc_color(mouth,  lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(mouth,  7,              LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(mouth,    LV_OPA_TRANSP,  LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mouth,     LV_OPA_TRANSP,  0);
    lv_obj_set_style_border_width(mouth, 0, 0);
    lv_obj_set_style_shadow_width(mouth, 0, 0);
    lv_obj_set_style_opa(mouth, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(mouth, LV_OBJ_FLAG_CLICKABLE);

    /* ── Label ── */
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "DRYAD");
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
}

/* ──────────────────────────────────────────────────────────
 * Display task
 * ────────────────────────────────────────────────────────── */

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

    create_robot_face();
    ESP_LOGI(TAG, "Robot face drawn");

    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
