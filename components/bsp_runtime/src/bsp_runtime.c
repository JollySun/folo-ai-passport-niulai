// SPDX-License-Identifier: MIT

#include "bsp_runtime.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

esp_err_t bsp_task_start(bsp_task_fn_t entry, const char *name,
                         uint32_t stack_size, uint32_t priority,
                         void *argument)
{
    if (!entry || !name || stack_size == 0) return ESP_ERR_INVALID_ARG;
    return xTaskCreate(entry, name, stack_size, argument, priority, NULL) == pdPASS
        ? ESP_OK : ESP_ERR_NO_MEM;
}

void bsp_delay_ms(uint32_t milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

bsp_queue_t bsp_queue_create(size_t item_size, size_t length)
{
    if (item_size == 0 || length == 0) return NULL;
    return xQueueCreate(length, item_size);
}

void bsp_queue_delete(bsp_queue_t queue)
{
    if (queue) vQueueDelete((QueueHandle_t)queue);
}

bool bsp_queue_overwrite(bsp_queue_t queue, const void *item)
{
    return queue && item &&
           xQueueOverwrite((QueueHandle_t)queue, item) == pdTRUE;
}

bool bsp_queue_receive(bsp_queue_t queue, void *item, uint32_t timeout_ms)
{
    if (!queue || !item) return false;
    TickType_t timeout = timeout_ms == UINT32_MAX
                       ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive((QueueHandle_t)queue, item, timeout) == pdTRUE;
}

bsp_mutex_t bsp_mutex_create(void)
{
    return xSemaphoreCreateMutex();
}

void bsp_mutex_delete(bsp_mutex_t mutex)
{
    if (mutex) vSemaphoreDelete((SemaphoreHandle_t)mutex);
}

bool bsp_mutex_lock(bsp_mutex_t mutex, uint32_t timeout_ms)
{
    if (!mutex) return false;
    TickType_t timeout = timeout_ms == UINT32_MAX
                       ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)mutex, timeout) == pdTRUE;
}

void bsp_mutex_unlock(bsp_mutex_t mutex)
{
    if (mutex) xSemaphoreGive((SemaphoreHandle_t)mutex);
}

void bsp_log_write(uint32_t level, const char *tag, const char *message)
{
    if (!tag || !message) return;
    switch (level) {
    case 1: ESP_LOGE(tag, "%s", message); break;
    case 2: ESP_LOGW(tag, "%s", message); break;
    case 3: ESP_LOGI(tag, "%s", message); break;
    default: ESP_LOGD(tag, "%s", message); break;
    }
}
