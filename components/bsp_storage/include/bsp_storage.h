// SPDX-License-Identifier: MIT

#pragma once

#include "esp_err.h"
#include <stddef.h>

typedef const void *bsp_storage_t;

bsp_storage_t bsp_storage_open(const char *label);
size_t bsp_storage_size(bsp_storage_t storage);
esp_err_t bsp_storage_read(bsp_storage_t storage, size_t offset,
                           void *buffer, size_t bytes);
esp_err_t bsp_storage_write(bsp_storage_t storage, size_t offset,
                            const void *buffer, size_t bytes);
esp_err_t bsp_storage_erase(bsp_storage_t storage, size_t offset,
                            size_t bytes);
