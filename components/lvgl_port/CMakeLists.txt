idf_component_register(SRCS "lvgl_port_display.c" "lvgl_port_indev.c"
                    INCLUDE_DIRS "include" "."
                    REQUIRES lvgl lcd_driver button_handler
                    PRIV_REQUIRES esp_timer)