#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct lv_obj_t {
    unsigned int id;
    int x;
    int width;
    uint32_t border_color;
    uint32_t text_color;
    int text_align;
    bool recolor;
    char text[96];
} lv_obj_t;

typedef struct _lv_display_t {
    unsigned int unused;
} lv_display_t;

typedef struct lv_timer_t {
    unsigned int unused;
} lv_timer_t;

typedef struct lv_draw_buf_t {
    unsigned int unused;
} lv_draw_buf_t;

typedef struct lv_font_t lv_font_t;

typedef struct {
    const lv_font_t *resolved_font;
    uint16_t adv_w;
    uint16_t box_w;
    uint16_t box_h;
    int16_t ofs_x;
    int16_t ofs_y;
    uint16_t stride;
    uint8_t format;
    uint8_t is_placeholder : 1;
    union {
        uint32_t index;
        const void *src;
    } gid;
} lv_font_glyph_dsc_t;

typedef struct {
    uint32_t bitmap_index : 20;
    uint32_t adv_w : 12;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;
} lv_font_fmt_txt_glyph_dsc_t;

typedef enum {
    LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL,
    LV_FONT_FMT_TXT_CMAP_SPARSE_FULL,
    LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY,
    LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
} lv_font_fmt_txt_cmap_type_t;

typedef struct {
    uint32_t range_start;
    uint16_t range_length;
    uint16_t glyph_id_start;
    const uint16_t *unicode_list;
    const void *glyph_id_ofs_list;
    uint16_t list_length;
    lv_font_fmt_txt_cmap_type_t type;
} lv_font_fmt_txt_cmap_t;

typedef struct {
    const uint8_t *glyph_bitmap;
    const lv_font_fmt_txt_glyph_dsc_t *glyph_dsc;
    const lv_font_fmt_txt_cmap_t *cmaps;
    const void *kern_dsc;
    uint16_t kern_scale;
    uint16_t cmap_num : 9;
    uint16_t bpp : 4;
    uint16_t kern_classes : 1;
    uint16_t bitmap_format : 2;
    uint8_t stride;
} lv_font_fmt_txt_dsc_t;

struct lv_font_t {
    bool (*get_glyph_dsc)(const lv_font_t *font,
                          lv_font_glyph_dsc_t *glyph,
                          uint32_t letter, uint32_t next_letter);
    const void *(*get_glyph_bitmap)(lv_font_glyph_dsc_t *glyph,
                                    lv_draw_buf_t *draw_buf);
    int32_t line_height;
    int32_t base_line;
    uint8_t subpx : 2;
    int8_t underline_position;
    int8_t underline_thickness;
    const void *dsc;
    const lv_font_t *fallback;
    void *user_data;
};

typedef uint32_t lv_color_t;
typedef uint8_t lv_opa_t;
typedef int lv_text_align_t;

typedef struct {
    struct {
        uint32_t magic;
        uint32_t cf;
        uint32_t flags;
        uint32_t w;
        uint32_t h;
        uint32_t stride;
    } header;
    size_t data_size;
    const uint8_t *data;
} lv_image_dsc_t;

#define LV_IMAGE_HEADER_MAGIC 0x19
#define LV_COLOR_FORMAT_RGB565 1
#define LV_LABEL_LONG_WRAP 1
#define LV_OBJ_FLAG_SCROLLABLE 0x01
#define LV_OBJ_FLAG_HIDDEN 0x02
#define LV_OPA_TRANSP 0
#define LV_OPA_30 77
#define LV_OPA_40 102
#define LV_OPA_60 153
#define LV_OPA_80 204
#define LV_OPA_90 230
#define LV_OPA_COVER 255
#define LV_RADIUS_CIRCLE 32767
#define LV_TEXT_ALIGN_LEFT 0
#define LV_TEXT_ALIGN_CENTER 1
#define LV_TEXT_ALIGN_RIGHT 2
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_FONT_FMT_TXT_LARGE 0
#define LV_FONT_SUBPX_NONE 0
#define LVGL_VERSION_MAJOR 9
#define LVGL_VERSION_MINOR 5
#define LV_VERSION_CHECK(major, minor, patch) 1

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t *font,
                                   lv_font_glyph_dsc_t *glyph,
                                   uint32_t letter, uint32_t next_letter);
const void *lv_font_get_bitmap_fmt_txt(lv_font_glyph_dsc_t *glyph,
                                       lv_draw_buf_t *draw_buf);

lv_color_t lv_color_hex(uint32_t color);
lv_obj_t *lv_obj_create(lv_obj_t *parent);
lv_obj_t *lv_image_create(lv_obj_t *parent);
lv_obj_t *lv_label_create(lv_obj_t *parent);
void lv_label_set_long_mode(lv_obj_t *label, int mode);
void lv_label_set_recolor(lv_obj_t *label, bool enabled);
void lv_label_set_text(lv_obj_t *label, const char *text);
void lv_label_set_text_fmt(lv_obj_t *label, const char *format, ...);
void lv_image_set_src(lv_obj_t *image, const void *source);
void lv_obj_add_flag(lv_obj_t *object, uint32_t flag);
void lv_obj_remove_flag(lv_obj_t *object, uint32_t flag);
void lv_obj_center(lv_obj_t *object);
void lv_obj_fade_in(lv_obj_t *object, uint32_t duration, uint32_t delay);
void lv_obj_set_pos(lv_obj_t *object, int x, int y);
void lv_obj_set_size(lv_obj_t *object, int width, int height);
void lv_obj_set_width(lv_obj_t *object, int width);
void lv_obj_set_style_bg_color(lv_obj_t *object, lv_color_t color, int selector);
void lv_obj_set_style_bg_opa(lv_obj_t *object, lv_opa_t opacity, int selector);
void lv_obj_set_style_border_color(lv_obj_t *object, lv_color_t color, int selector);
void lv_obj_set_style_border_width(lv_obj_t *object, int width, int selector);
void lv_obj_set_style_pad_all(lv_obj_t *object, int padding, int selector);
void lv_obj_set_style_radius(lv_obj_t *object, int radius, int selector);
void lv_obj_set_style_shadow_color(lv_obj_t *object, lv_color_t color, int selector);
void lv_obj_set_style_shadow_offset_y(lv_obj_t *object, int offset, int selector);
void lv_obj_set_style_shadow_opa(lv_obj_t *object, lv_opa_t opacity, int selector);
void lv_obj_set_style_shadow_width(lv_obj_t *object, int width, int selector);
void lv_obj_set_style_text_align(lv_obj_t *object, lv_text_align_t align, int selector);
void lv_obj_set_style_text_color(lv_obj_t *object, lv_color_t color, int selector);
void lv_obj_set_style_text_font(lv_obj_t *object, const lv_font_t *font, int selector);
void lv_obj_set_style_text_line_space(lv_obj_t *object, int space, int selector);
void lv_screen_load(lv_obj_t *screen);
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t period,
                            void *user_data);
void lv_timer_pause(lv_timer_t *timer);
void lv_timer_resume(lv_timer_t *timer);
void lv_timer_ready(lv_timer_t *timer);
