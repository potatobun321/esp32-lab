idf_component_register(
    SRCS "hello_world_main.c" "oled_graphics.c" "scenes.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_i2c esp_driver_gpio esp_lcd esp_timer
)
