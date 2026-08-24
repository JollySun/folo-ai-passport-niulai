idf_component_register(
    SRCS "main.c"
         "niulai_app.c"
         "niulai_font_12.c"
         "niulai_font_16.c"
         "niulai_font_22.c"
         "niulai_voice_store.c"
    INCLUDE_DIRS "."
    REQUIRES bsp passport-rust esp_partition
    EMBED_FILES "assets/niulai/home.rgb565"
                "assets/niulai/calf-1.rgb565"
                "assets/niulai/calf-2.rgb565"
                "assets/niulai/mother-1.rgb565"
                "assets/niulai/mother-2-open.rgb565"
                "assets/niulai/mama.pcm"
                "assets/niulai/niulai.pcm"
)
