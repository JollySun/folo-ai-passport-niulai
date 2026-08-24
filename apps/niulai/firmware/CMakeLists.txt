idf_component_register(
    SRCS "app_main.c"
         "niulai_app.c"
         "niulai_font_12.c"
         "niulai_font_16.c"
         "niulai_font_22.c"
         "niulai_voice_store.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_partition
    EMBED_FILES "assets/home.rgb565"
                "assets/calf-1.rgb565"
                "assets/calf-2.rgb565"
                "assets/mother-1.rgb565"
                "assets/mother-2-open.rgb565"
                "assets/mama.pcm"
                "assets/niulai.pcm"
)

if(NOT CONFIG_IDF_TARGET_ESP32C3)
    message(FATAL_ERROR "The Niu Lai firmware currently supports only ESP32-C3")
endif()

set(RUST_TARGET "riscv32imc-esp-espidf")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_BUILD_TYPE "debug")
    set(CARGO_BUILD_ARG "")
else()
    set(CARGO_BUILD_TYPE "release")
    set(CARGO_BUILD_ARG "--release")
endif()

set(CARGO_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(CARGO_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/target")
set(RUST_STATIC_LIBRARY
    "${CARGO_TARGET_DIR}/${RUST_TARGET}/${CARGO_BUILD_TYPE}/libniulai_app.a")

ExternalProject_Add(
    project_niulai_app
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    USES_TERMINAL_BUILD true
    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        cargo build
            --locked
            --package niulai-app
            --target ${RUST_TARGET}
            --target-dir ${CARGO_TARGET_DIR}
            ${CARGO_BUILD_ARG}
            -Zbuild-std=core
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
    SOURCE_DIR "${CARGO_PROJECT_DIR}"
    BINARY_DIR "${CARGO_PROJECT_DIR}"
    BUILD_BYPRODUCTS "${RUST_STATIC_LIBRARY}"
)

add_prebuilt_library(niulai_app_rust_library "${RUST_STATIC_LIBRARY}")
add_dependencies(niulai_app_rust_library project_niulai_app)
target_link_libraries(${COMPONENT_LIB} PRIVATE niulai_app_rust_library)
