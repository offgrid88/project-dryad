idf_component_register(
    SRCS "main.c"
         "moisture_sensor.c"
         "display.c"
    INCLUDE_DIRS "."
    REQUIRES "driver"
             "esp_adc"
             "freertos"
)