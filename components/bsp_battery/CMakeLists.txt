idf_component_register(
    SRCS "src/bsp_battery.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "${PROJECT_DIR}/crates/passport-core/include"
    PRIV_REQUIRES bsp_board bsp_i2c
)
