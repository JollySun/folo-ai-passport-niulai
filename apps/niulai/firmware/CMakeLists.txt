idf_component_register(
    SRCS "app_main.c"
         "niulai_app.c"
         "niulai_font_12.c"
         "niulai_font_16.c"
         "niulai_font_22.c"
         "niulai_voice_store.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "${PROJECT_DIR}/crates/passport-core/include"
    REQUIRES bsp_board bsp_i2c bsp_display bsp_lvgl bsp_button bsp_audio bsp_battery esp_partition lvgl__lvgl
    EMBED_FILES "assets/home.rgb565"
                "assets/calf-1.rgb565"
                "assets/calf-2.rgb565"
                "assets/mother-1.rgb565"
                "assets/mother-2-open.rgb565"
                "assets/mama.pcm"
                "assets/niulai.pcm"
)

include("${PROJECT_DIR}/cmake/passport_rust_app.cmake")
passport_add_rust_staticlib(PACKAGE niulai-app LIBRARY niulai_app)
