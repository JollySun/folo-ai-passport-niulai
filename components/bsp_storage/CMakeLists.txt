idf_component_register(
    SRCS "src/bsp_storage.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES esp_partition
)
