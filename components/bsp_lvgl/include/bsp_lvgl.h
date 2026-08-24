// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// LVGL integration requires bsp_display_init() to succeed first. The forward
// declaration keeps LVGL headers out of callers that only need the lock.
struct _lv_display_t;

struct _lv_display_t *bsp_lvgl_init(void);
bool bsp_lvgl_lock(int timeout_ms);
void bsp_lvgl_unlock(void);

typedef void bsp_ui_object_t;
typedef void bsp_ui_image_source_t;
typedef void (*bsp_ui_timer_cb_t)(void);

typedef enum {
    BSP_UI_ALIGN_LEFT = 0,
    BSP_UI_ALIGN_CENTER,
    BSP_UI_ALIGN_RIGHT,
} bsp_ui_text_align_t;

bsp_ui_object_t *bsp_ui_screen_create(void);
bsp_ui_object_t *bsp_ui_panel_create(bsp_ui_object_t *parent);
bsp_ui_object_t *bsp_ui_label_create(bsp_ui_object_t *parent,
                                     const void *font, uint32_t color);
bsp_ui_object_t *bsp_ui_image_create(bsp_ui_object_t *parent);
bsp_ui_image_source_t *bsp_ui_image_source_create(const void *pixels,
                                                   uint16_t width,
                                                   uint16_t height);
void bsp_ui_screen_load(bsp_ui_object_t *screen);
void bsp_ui_timer_create(bsp_ui_timer_cb_t callback, uint32_t period_ms);

void bsp_ui_set_pos(bsp_ui_object_t *object, int32_t x, int32_t y);
void bsp_ui_set_size(bsp_ui_object_t *object, int32_t width, int32_t height);
void bsp_ui_set_width(bsp_ui_object_t *object, int32_t width);
void bsp_ui_set_hidden(bsp_ui_object_t *object, bool hidden);
void bsp_ui_set_scrollable(bsp_ui_object_t *object, bool scrollable);
void bsp_ui_center(bsp_ui_object_t *object);
void bsp_ui_fade_in(bsp_ui_object_t *object, uint32_t duration_ms);

void bsp_ui_set_background(bsp_ui_object_t *object, uint32_t color,
                           uint8_t opacity);
void bsp_ui_set_border(bsp_ui_object_t *object, uint32_t color,
                       int32_t width);
void bsp_ui_set_radius(bsp_ui_object_t *object, int32_t radius);
void bsp_ui_set_circle(bsp_ui_object_t *object);
void bsp_ui_set_padding(bsp_ui_object_t *object, int32_t padding);
void bsp_ui_set_shadow(bsp_ui_object_t *object, uint32_t color,
                       uint8_t opacity, int32_t width, int32_t offset_y);

void bsp_ui_label_set_layout(bsp_ui_object_t *label, const void *font,
                             uint32_t color, bsp_ui_text_align_t align,
                             int32_t line_space);
void bsp_ui_label_set_text(bsp_ui_object_t *label, const char *text);
void bsp_ui_image_set_source(bsp_ui_object_t *image,
                             const bsp_ui_image_source_t *source);
