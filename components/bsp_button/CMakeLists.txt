idf_component_register(
    SRCS "src/bsp_button.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES bsp_board button esp_adc
)
