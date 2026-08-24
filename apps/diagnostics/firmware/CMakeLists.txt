idf_component_register(
    REQUIRES rust_app_entry bsp_runtime bsp_i2c bsp_battery
)

include("${PROJECT_DIR}/cmake/passport_rust_app.cmake")
passport_add_rust_staticlib(PACKAGE diagnostics-app LIBRARY diagnostics_app)
