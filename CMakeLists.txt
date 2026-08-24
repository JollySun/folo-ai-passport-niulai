cmake_minimum_required(VERSION 3.16)

set(PASSPORT_APP "" CACHE STRING "Application directory name under apps/")
if(PASSPORT_APP STREQUAL "")
    message(FATAL_ERROR
        "PASSPORT_APP is required. Build with idf.py -D PASSPORT_APP=<app> build")
endif()
if(NOT PASSPORT_APP MATCHES "^[a-z0-9][a-z0-9_-]*$")
    message(FATAL_ERROR "Invalid PASSPORT_APP: ${PASSPORT_APP}")
endif()

set(PASSPORT_APP_COMPONENT "${CMAKE_CURRENT_LIST_DIR}/apps/${PASSPORT_APP}/firmware")
if(NOT EXISTS "${PASSPORT_APP_COMPONENT}/CMakeLists.txt")
    message(FATAL_ERROR
        "Application '${PASSPORT_APP}' has no firmware component at ${PASSPORT_APP_COMPONENT}")
endif()
list(APPEND EXTRA_COMPONENT_DIRS "${PASSPORT_APP_COMPONENT}")
# Build only the selected firmware module and the transitive dependencies it
# names. Without this, ESP-IDF builds every reusable BSP capability.
set(COMPONENTS "firmware")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FoloToy-AI-Passport)
