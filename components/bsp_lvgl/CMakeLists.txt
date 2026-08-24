idf_component_register(
    SRCS "src/bsp_display_lvgl.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_display
    PRIV_REQUIRES bsp_board esp_lvgl_port
)
