idf_component_register(
    SRCS "src/bsp_display.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd
    PRIV_REQUIRES bsp_board driver
)
