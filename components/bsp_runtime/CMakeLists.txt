idf_component_register(
    SRCS "src/bsp_runtime.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES freertos log
)
