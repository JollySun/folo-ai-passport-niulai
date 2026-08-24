// SPDX-License-Identifier: MIT

#include "bsp_storage.h"

#include "esp_partition.h"

bsp_storage_t bsp_storage_open(const char *label)
{
    if (!label) return NULL;
    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                    ESP_PARTITION_SUBTYPE_ANY, label);
}

size_t bsp_storage_size(bsp_storage_t storage)
{
    const esp_partition_t *partition = storage;
    return partition ? partition->size : 0;
}

esp_err_t bsp_storage_read(bsp_storage_t storage, size_t offset,
                           void *buffer, size_t bytes)
{
    if (!storage || (!buffer && bytes != 0)) return ESP_ERR_INVALID_ARG;
    return esp_partition_read(storage, offset, buffer, bytes);
}

esp_err_t bsp_storage_write(bsp_storage_t storage, size_t offset,
                            const void *buffer, size_t bytes)
{
    if (!storage || (!buffer && bytes != 0)) return ESP_ERR_INVALID_ARG;
    return esp_partition_write(storage, offset, buffer, bytes);
}

esp_err_t bsp_storage_erase(bsp_storage_t storage, size_t offset,
                            size_t bytes)
{
    if (!storage) return ESP_ERR_INVALID_ARG;
    return esp_partition_erase_range(storage, offset, bytes);
}
