#pragma once

#include "FreeRTOS.h"

BaseType_t xTaskCreate(void (*task)(void *), const char *name,
                       uint32_t stack_depth, void *argument,
                       UBaseType_t priority, TaskHandle_t *handle);
void vTaskDelay(TickType_t ticks);
