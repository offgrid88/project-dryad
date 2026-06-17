idf_component_register(SRCS "lvgl_port_display.c" "lvgl_port_indev.c"
                    INCLUDE_DIRS "include" "."
                    REQUIRES lvgl lcd_driver button_handler
                    PRIV_REQUIRES esp_timer)

# Expose lv_conf.h (in this directory) to all components including LVGL itself
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
idf_build_set_property(INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" APPEND)