idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES lcd_driver button_handler lvgl_port)