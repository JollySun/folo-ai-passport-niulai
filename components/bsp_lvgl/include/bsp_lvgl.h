// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include <stdbool.h>

// LVGL integration requires bsp_display_init() to succeed first. The forward
// declaration keeps LVGL headers out of callers that only need the lock.
struct _lv_display_t;

struct _lv_display_t *bsp_lvgl_init(void);
bool bsp_lvgl_lock(int timeout_ms);
void bsp_lvgl_unlock(void);
