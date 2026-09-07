#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

struct _lv_display_t;

esp_err_t bsp_display_init(void);
struct _lv_display_t *bsp_lvgl_init(void);
void bsp_display_backlight(uint8_t percent);
esp_err_t bsp_display_sleep(void);
esp_err_t bsp_display_wake(void);
bool bsp_lvgl_lock(uint32_t timeout_ms);
void bsp_lvgl_unlock(void);
