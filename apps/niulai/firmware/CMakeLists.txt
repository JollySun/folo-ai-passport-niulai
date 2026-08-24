idf_component_register(
    SRCS "niulai_font_12.c"
         "niulai_font_16.c"
         "niulai_font_22.c"
    INCLUDE_DIRS "."
    REQUIRES rust_app_entry bsp_runtime bsp_storage bsp_board bsp_i2c bsp_display bsp_lvgl bsp_button bsp_audio bsp_battery lvgl__lvgl
    EMBED_FILES "assets/home.rgb565"
                "assets/calf-1.rgb565"
                "assets/calf-2.rgb565"
                "assets/mother-1.rgb565"
                "assets/mother-2-open.rgb565"
                "assets/mama.pcm"
                "assets/niulai.pcm"
    WHOLE_ARCHIVE
)

include("${PROJECT_DIR}/cmake/passport_rust_app.cmake")
passport_add_rust_staticlib(PACKAGE niulai-app LIBRARY niulai_app)
