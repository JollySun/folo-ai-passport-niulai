function(passport_add_rust_staticlib)
    cmake_parse_arguments(RUST "" "PACKAGE;LIBRARY" "" ${ARGN})
    if(NOT RUST_PACKAGE OR NOT RUST_LIBRARY)
        message(FATAL_ERROR
            "passport_add_rust_staticlib requires PACKAGE and LIBRARY")
    endif()
    if(NOT CONFIG_IDF_TARGET_ESP32C3)
        message(FATAL_ERROR "Rust firmware currently supports only ESP32-C3")
    endif()

    set(rust_target "riscv32imc-esp-espidf")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(cargo_build_type "debug")
        set(cargo_release_arg "")
    else()
        set(cargo_build_type "release")
        set(cargo_release_arg "--release")
    endif()

    set(cargo_target_dir "${CMAKE_CURRENT_BINARY_DIR}/rust-target")
    set(rust_static_library
        "${cargo_target_dir}/${rust_target}/${cargo_build_type}/lib${RUST_LIBRARY}.a")
    set(rust_project "rust_${RUST_LIBRARY}")
    set(rust_prebuilt "${RUST_LIBRARY}_rust_library")

    ExternalProject_Add(
        ${rust_project}
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/${rust_project}"
        DOWNLOAD_COMMAND ""
        CONFIGURE_COMMAND ""
        USES_TERMINAL_BUILD true
        BUILD_COMMAND ${CMAKE_COMMAND} -E env
            cargo build
                --manifest-path "${PROJECT_DIR}/Cargo.toml"
                --locked
                --package ${RUST_PACKAGE}
                --target ${rust_target}
                --target-dir ${cargo_target_dir}
                ${cargo_release_arg}
                -Zbuild-std=core
        INSTALL_COMMAND ""
        BUILD_ALWAYS TRUE
        SOURCE_DIR "${PROJECT_DIR}"
        BINARY_DIR "${PROJECT_DIR}"
        BUILD_BYPRODUCTS "${rust_static_library}"
    )

    add_prebuilt_library(${rust_prebuilt} "${rust_static_library}")
    add_dependencies(${rust_prebuilt} ${rust_project})
    target_link_libraries(${COMPONENT_LIB} PRIVATE ${rust_prebuilt})
endfunction()
