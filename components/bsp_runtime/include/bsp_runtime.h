// SPDX-License-Identifier: MIT

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*bsp_task_fn_t)(void *argument);
typedef void *bsp_queue_t;
typedef void *bsp_mutex_t;

esp_err_t bsp_task_start(bsp_task_fn_t entry, const char *name,
                         uint32_t stack_size, uint32_t priority,
                         void *argument);
void bsp_delay_ms(uint32_t milliseconds);

bsp_queue_t bsp_queue_create(size_t item_size, size_t length);
void bsp_queue_delete(bsp_queue_t queue);
bool bsp_queue_overwrite(bsp_queue_t queue, const void *item);
bool bsp_queue_receive(bsp_queue_t queue, void *item, uint32_t timeout_ms);

bsp_mutex_t bsp_mutex_create(void);
void bsp_mutex_delete(bsp_mutex_t mutex);
bool bsp_mutex_lock(bsp_mutex_t mutex, uint32_t timeout_ms);
void bsp_mutex_unlock(bsp_mutex_t mutex);

void bsp_log_write(uint32_t level, const char *tag, const char *message);
