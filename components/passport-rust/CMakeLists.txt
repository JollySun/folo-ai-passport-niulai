idf_component_register(SRCS "placeholder.c")

if(NOT CONFIG_IDF_TARGET_ESP32C3)
    message(FATAL_ERROR "passport-rust currently supports only ESP32-C3")
endif()

set(RUST_TARGET "riscv32imc-esp-espidf")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_BUILD_TYPE "debug")
    set(CARGO_BUILD_ARG "")
else()
    set(CARGO_BUILD_TYPE "release")
    set(CARGO_BUILD_ARG "--release")
endif()

set(CARGO_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CARGO_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/target")
set(RUST_STATIC_LIBRARY
    "${CARGO_TARGET_DIR}/${RUST_TARGET}/${CARGO_BUILD_TYPE}/libpassport_rust.a")

ExternalProject_Add(
    project_passport_rust
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    USES_TERMINAL_BUILD true
    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        cargo build
            --locked
            --package passport-rust
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

add_prebuilt_library(passport_rust_library "${RUST_STATIC_LIBRARY}")
add_dependencies(passport_rust_library project_passport_rust)
target_link_libraries(${COMPONENT_LIB} PRIVATE passport_rust_library)
