idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp_i2c bsp_battery
)

include("${PROJECT_DIR}/cmake/passport_rust_app.cmake")
passport_add_rust_staticlib(PACKAGE diagnostics-app LIBRARY diagnostics_app)
