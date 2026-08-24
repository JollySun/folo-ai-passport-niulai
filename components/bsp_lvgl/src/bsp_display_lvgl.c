// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

// components/bsp_lvgl/src/bsp_display_lvgl.c
#include "bsp_display.h"
#include "bsp_lvgl.h"
#include "bsp_pins.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "bsp_lvgl";

static lv_display_t *s_disp;

typedef struct {
    bsp_ui_timer_cb_t callback;
} ui_timer_context_t;

static void ui_timer_bridge(lv_timer_t *timer)
{
    ui_timer_context_t *context = lv_timer_get_user_data(timer);
    if (context && context->callback) context->callback();
}

lv_display_t *bsp_lvgl_init(void) {
    if (s_disp) return s_disp;
    if (!bsp_display_panel()) {
        ESP_LOGE(TAG, "请先成功调用 bsp_display_init()");
        return NULL;
    }

    const lvgl_port_cfg_t pc = ESP_LVGL_PORT_INIT_CONFIG();
    if (lvgl_port_init(&pc) != ESP_OK) {
        ESP_LOGE(TAG, "lvgl_port_init 失败");
        return NULL;
    }

    const lvgl_port_display_cfg_t dc = {
        .panel_handle = bsp_display_panel(),
        .io_handle    = bsp_display_io(),
        // ⚠ C3 无 PSRAM,DMA 只能用内部 RAM(总共约 150KB)。
        // 20 行单缓冲 ≈ 9.6KB;若改成 40 行双缓冲(≈37.5KB)会把 I2S 等外设的
        // DMA 描述符挤到 NO_MEM。刷新略慢但稳。
        .buffer_size   = (uint32_t)BSP_LCD_W * 20,
        .double_buffer = false,
        .hres = BSP_LCD_W, .vres = BSP_LCD_H,
        // 旋转/镜像必须在这里配:esp_lvgl_port 注册显示时会重新下发 MADCTL,
        // 覆盖 bsp_display.c 里 esp_lcd_panel_mirror() 的设置。
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        // swap_bytes:LVGL 输出小端 RGB565,ST7789 走 SPI 要大端 → 需交换高低字节。
        .flags = { .buff_dma = true, .swap_bytes = true },
    };
    s_disp = lvgl_port_add_disp(&dc);
    if (!s_disp) { ESP_LOGE(TAG, "lvgl_port_add_disp 失败"); return NULL; }

    ESP_LOGI(TAG, "LVGL 就绪");
    return s_disp;
}

bool bsp_lvgl_lock(int timeout_ms) { return lvgl_port_lock(timeout_ms); }
void bsp_lvgl_unlock(void)         { lvgl_port_unlock(); }

bsp_ui_object_t *bsp_ui_screen_create(void)
{
    return lv_obj_create(NULL);
}

bsp_ui_object_t *bsp_ui_panel_create(bsp_ui_object_t *parent)
{
    return lv_obj_create(parent);
}

bsp_ui_object_t *bsp_ui_label_create(bsp_ui_object_t *parent,
                                     const void *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

bsp_ui_object_t *bsp_ui_image_create(bsp_ui_object_t *parent)
{
    return lv_image_create(parent);
}

bsp_ui_image_source_t *bsp_ui_image_source_create(const void *pixels,
                                                   uint16_t width,
                                                   uint16_t height)
{
    if (!pixels || width == 0 || height == 0) return NULL;
    lv_image_dsc_t *source = lv_malloc(sizeof(*source));
    if (!source) return NULL;
    *source = (lv_image_dsc_t) {
        .header.magic = LV_IMAGE_HEADER_MAGIC,
        .header.cf = LV_COLOR_FORMAT_RGB565,
        .header.flags = 0,
        .header.w = width,
        .header.h = height,
        .header.stride = width * 2,
        .data_size = (uint32_t)width * height * 2,
        .data = pixels,
    };
    return (bsp_ui_image_source_t *)source;
}

void bsp_ui_screen_load(bsp_ui_object_t *screen)
{
    if (screen) lv_screen_load(screen);
}

void bsp_ui_timer_create(bsp_ui_timer_cb_t callback, uint32_t period_ms)
{
    if (!callback || period_ms == 0) return;
    ui_timer_context_t *context = lv_malloc(sizeof(*context));
    if (!context) return;
    context->callback = callback;
    if (!lv_timer_create(ui_timer_bridge, period_ms, context)) lv_free(context);
}

void bsp_ui_set_pos(bsp_ui_object_t *object, int32_t x, int32_t y)
{
    if (object) lv_obj_set_pos(object, x, y);
}

void bsp_ui_set_size(bsp_ui_object_t *object, int32_t width, int32_t height)
{
    if (object) lv_obj_set_size(object, width, height);
}

void bsp_ui_set_width(bsp_ui_object_t *object, int32_t width)
{
    if (object) lv_obj_set_width(object, width);
}

void bsp_ui_set_hidden(bsp_ui_object_t *object, bool hidden)
{
    if (!object) return;
    if (hidden) lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
}

void bsp_ui_set_scrollable(bsp_ui_object_t *object, bool scrollable)
{
    if (!object) return;
    if (scrollable) lv_obj_add_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    else lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

void bsp_ui_center(bsp_ui_object_t *object)
{
    if (object) lv_obj_center(object);
}

void bsp_ui_fade_in(bsp_ui_object_t *object, uint32_t duration_ms)
{
    if (object) lv_obj_fade_in(object, duration_ms, 0);
}

void bsp_ui_set_background(bsp_ui_object_t *object, uint32_t color,
                           uint8_t opacity)
{
    if (!object) return;
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(object, opacity, 0);
}

void bsp_ui_set_border(bsp_ui_object_t *object, uint32_t color, int32_t width)
{
    if (!object) return;
    lv_obj_set_style_border_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(object, width, 0);
}

void bsp_ui_set_radius(bsp_ui_object_t *object, int32_t radius)
{
    if (object) lv_obj_set_style_radius(object, radius, 0);
}

void bsp_ui_set_circle(bsp_ui_object_t *object)
{
    if (object) lv_obj_set_style_radius(object, LV_RADIUS_CIRCLE, 0);
}

void bsp_ui_set_padding(bsp_ui_object_t *object, int32_t padding)
{
    if (object) lv_obj_set_style_pad_all(object, padding, 0);
}

void bsp_ui_set_shadow(bsp_ui_object_t *object, uint32_t color,
                       uint8_t opacity, int32_t width, int32_t offset_y)
{
    if (!object) return;
    lv_obj_set_style_shadow_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_shadow_opa(object, opacity, 0);
    lv_obj_set_style_shadow_width(object, width, 0);
    lv_obj_set_style_shadow_offset_y(object, offset_y, 0);
}

void bsp_ui_label_set_layout(bsp_ui_object_t *label, const void *font,
                             uint32_t color, bsp_ui_text_align_t align,
                             int32_t line_space)
{
    if (!label) return;
    lv_text_align_t lv_align = LV_TEXT_ALIGN_LEFT;
    if (align == BSP_UI_ALIGN_CENTER) lv_align = LV_TEXT_ALIGN_CENTER;
    else if (align == BSP_UI_ALIGN_RIGHT) lv_align = LV_TEXT_ALIGN_RIGHT;
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, lv_align, 0);
    lv_obj_set_style_text_line_space(label, line_space, 0);
}

void bsp_ui_label_set_text(bsp_ui_object_t *label, const char *text)
{
    if (label && text) lv_label_set_text(label, text);
}

void bsp_ui_image_set_source(bsp_ui_object_t *image,
                             const bsp_ui_image_source_t *source)
{
    if (image && source) lv_image_set_src(image, source);
}
