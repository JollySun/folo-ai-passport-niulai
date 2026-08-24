idf_component_register(
    SRCS "src/bsp_audio.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES bsp_board bsp_i2c esp_driver_i2s esp_codec_dev
)
