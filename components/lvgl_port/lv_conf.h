/**
 * @file lv_conf.h
 * @brief LVGL configuration file for ESP32-C3 with 240x240 circular LCD
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface */
#define LV_COLOR_16_SWAP 1

/* Enable features to draw on transparent background */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* 1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()` */
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    /* Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB) */
    #define LV_MEM_SIZE (32U * 1024U)

    /* Set an address for the memory pool instead of allocating it as a normal array */
    #define LV_MEM_ADR 0

    /* Automatically defragment on free */
    #define LV_MEM_AUTO_DEFRAG 1
#else
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/* Number of the memory buffer */
#define LV_MEM_BUF_MAX_NUM 16

/* Use standard `memcpy` and `memset` instead of LVGL's own functions */
#define LV_MEMCPY_MEMSET_STD 0

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Use a custom tick source */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000)
#endif

/* Default Dot Per Inch */
#define LV_DPI_DEF 130

/*=======================
   FEATURE CONFIGURATION
 *=======================*/

/*-------------
 * Drawing
 *-----------*/

/* Enable complex draw engine */
#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX != 0
    /* Allow buffering some shadow calculation */
    #define LV_SHADOW_CACHE_SIZE 0

    /* Set number of maximally cached circle data */
    #define LV_CIRCLE_CACHE_SIZE 4
#endif

/* Default gradient buffer size */
#define LV_GRAD_CACHE_DEF_SIZE 0

/* Enable anti-aliasing */
#define LV_USE_FONT_SUBPX 0

/* Set the maximum number of simultaneously created layers */
#define LV_LAYER_MAX_NUM 4

/* Enable/disable support for image decoder and cache */
#define LV_IMG_CACHE_DEF_SIZE 0

/*-------------
 * GPU
 *-----------*/

/* Enable STM32 DMA2D GPU */
#define LV_USE_GPU_STM32_DMA2D 0

/* Enable NXP PXP GPU */
#define LV_USE_GPU_NXP_PXP 0

/* Enable VG-Lite GPU */
#define LV_USE_GPU_NXP_VG_LITE 0

/* Enable SWM341 DMA2D GPU */
#define LV_USE_GPU_SWM341_DMA2D 0

/* Enable arm-2d GPU */
#define LV_USE_GPU_ARM2D 0

/*-------------
 * Logging
 *-----------*/

/* Enable the log module */
#define LV_USE_LOG 1
#if LV_USE_LOG
    /* Log level */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /* 1: Print logs with 'printf', 0: User needs to implement `lv_log_print_cb` */
    #define LV_LOG_PRINTF 1

    /* Enable/disable LV_LOG_TRACE in modules */
    #define LV_LOG_TRACE_MEM        0
    #define LV_LOG_TRACE_TIMER      0
    #define LV_LOG_TRACE_INDEV      0
    #define LV_LOG_TRACE_DISP_REFR  0
    #define LV_LOG_TRACE_EVENT      0
    #define LV_LOG_TRACE_OBJ_CREATE 0
    #define LV_LOG_TRACE_LAYOUT     0
    #define LV_LOG_TRACE_ANIM       0
#endif

/*-------------
 * Asserts
 *-----------*/

/* Enable asserts */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* Handler for assert failures */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);

/*-------------
 * Others
 *-----------*/

/* 1: Show CPU usage and FPS count */
#define LV_USE_PERF_MONITOR 0
#if LV_USE_PERF_MONITOR
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#endif

/* 1: Show the used memory and the memory fragmentation */
#define LV_USE_MEM_MONITOR 0
#if LV_USE_MEM_MONITOR
    #define LV_USE_MEM_MONITOR_POS LV_ALIGN_BOTTOM_LEFT
#endif

/* 1: Draw random colored rectangles over the redrawn areas */
#define LV_USE_REFR_DEBUG 0

/* Maximum buffer size for printf */
#define LV_SPRINTF_MAX_BUFFER_SIZE 1024

/* Use a custom sprintf-like function */
#define LV_SPRINTF_CUSTOM 0

/* BIDI (bidirectional) text support */
#define LV_USE_BIDI 0

/* Arabic/Persian text processing */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 * FONT USAGE
 *===================*/

/* Montserrat fonts */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0

/* Pixel perfect monospace fonts */
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/* Optionally declare custom fonts here */
#define LV_FONT_CUSTOM_DECLARE

/* Set a default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Enable handling large font and/or fonts with a lot of characters */
#define LV_FONT_FMT_TXT_LARGE 0

/* Enables/disables support for compressed fonts */
#define LV_USE_FONT_COMPRESSED 0

/* Enable subpixel rendering */
#define LV_USE_FONT_SUBPX 0

/* Set the pixel order of the display */
#define LV_FONT_SUBPX_BGR 0

/*=================
 * TEXT SETTINGS
 *=================*/

/* Select a character encoding */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* Break long words */
#define LV_TXT_BREAK_CHARS " ,.;:-_"

/* Set a minimal word length for line break */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/* Put a line break where the text would overflow */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/* Minimum number of characters in a long word after a break */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* The control character to use for signaling text recoloring */
#define LV_TXT_COLOR_CMD "#"

/* Support bidirectional texts */
#define LV_USE_BIDI 0

/* Set the default text direction */
#define LV_TXT_DEFAULT_DIR LV_BASE_DIR_LTR

/* Enable Arabic/Persian processing */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*====================
 * WIDGET USAGE
 *====================*/

/* Documentation of the widgets: https://docs.lvgl.io/latest/widgets/index.html */

#define LV_USE_ARC        1

#define LV_USE_BAR        1

#define LV_USE_BTN        1

#define LV_USE_BTNMATRIX  1

#define LV_USE_CANVAS     0

#define LV_USE_CHECKBOX   1

#define LV_USE_DROPDOWN   1

#define LV_USE_IMG        1

#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1
    #define LV_LABEL_LONG_TXT_HINT 1
#endif

#define LV_USE_LINE       1

#define LV_USE_ROLLER     1
#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES 7
#endif

#define LV_USE_SLIDER     1

#define LV_USE_SWITCH     1

#define LV_USE_TEXTAREA   1
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#endif

#define LV_USE_TABLE      0

/*==================
 * EXTRA COMPONENTS
 *==================*/

/*-----------
 * Widgets
 *----------*/
#define LV_USE_ANIMIMG    0

#define LV_USE_CALENDAR   0

#define LV_USE_CHART      0

#define LV_USE_COLORWHEEL 0

#define LV_USE_IMGBTN     0

#define LV_USE_KEYBOARD   0

#define LV_USE_LED        1

#define LV_USE_LIST       1

#define LV_USE_MENU       0

#define LV_USE_METER      1

#define LV_USE_MSGBOX     1

#define LV_USE_SPAN       0

#define LV_USE_SPINBOX    1

#define LV_USE_SPINNER    1

#define LV_USE_TABVIEW    1

#define LV_USE_TILEVIEW   0

#define LV_USE_WIN        0

/*-----------
 * Themes
 *----------*/
/* A simple, impressive and very complete theme */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    /* 0: Light mode; 1: Dark mode */
    #define LV_THEME_DEFAULT_DARK 1

    /* 1: Enable grow on press */
    #define LV_THEME_DEFAULT_GROW 1

    /* Default transition time in [ms] */
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

/* A very simple theme that is a good starting point for a custom theme */
#define LV_USE_THEME_BASIC 0

/* A theme designed for monochrome displays */
#define LV_USE_THEME_MONO 0

/*-----------
 * Layouts
 *----------*/

/* A layout similar to Flexbox in CSS */
#define LV_USE_FLEX 1

/* A layout similar to Grid in CSS */
#define LV_USE_GRID 1

/*====================
 * EXAMPLES
 *====================*/

/* Enable the examples to be built with the library */
#define LV_BUILD_EXAMPLES 1

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/