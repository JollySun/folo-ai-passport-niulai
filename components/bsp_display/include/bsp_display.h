// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

// components/bsp_display/include/bsp_display.h
// ST7789P3 240x320 显示:SPI 面板初始化 + 厂商专属寄存器 + LEDC 背光调光。
#pragma once

#include "esp_err.h"
#include "esp_lcd_types.h"
#include <stdint.h>

// 初始化 SPI 总线、面板、厂商寄存器、背光 LEDC。成功后屏幕已上电但内容未定。
esp_err_t bsp_display_init(void);

// 取底层面板句柄。想直接 esp_lcd_panel_draw_bitmap 画,或接 LVGL 以外的 GUI 时用。
// 未初始化返回 NULL。
esp_lcd_panel_handle_t bsp_display_panel(void);

// 取底层 panel io 句柄(LVGL 接入需要)。未初始化返回 NULL。
esp_lcd_panel_io_handle_t bsp_display_io(void);

// 背光亮度 0..100(%)。LEDC PWM,0=全灭。
void bsp_display_backlight(uint8_t percent);
