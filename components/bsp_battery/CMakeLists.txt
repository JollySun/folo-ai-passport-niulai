idf_component_register(
    SRCS "src/bsp_battery.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES bsp_board bsp_i2c
)
