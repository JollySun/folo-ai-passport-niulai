// SPDX-License-Identifier: MIT

#include "bsp_button.h"
#include "lvgl.h"
#include "niulai_app.h"
#include "niulai_fonts.h"
#include "niulai_model.h"
#include "niulai_voice_store.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const uint8_t test_home_data[1] asm("_binary_home_rgb565_start") = {0};
const uint8_t test_calf_1_data[1] asm("_binary_calf_1_rgb565_start") = {0};
const uint8_t test_calf_2_data[1] asm("_binary_calf_2_rgb565_start") = {0};
const uint8_t test_mother_1_data[1] asm("_binary_mother_1_rgb565_start") = {0};
const uint8_t test_mother_2_data[1] asm("_binary_mother_2_open_rgb565_start") = {0};
const uint8_t test_mama_pcm_start[1] asm("_binary_mama_pcm_start") = {0};
const uint8_t test_mama_pcm_end[1] asm("_binary_mama_pcm_end") = {0};
const uint8_t test_niulai_pcm_start[1] asm("_binary_niulai_pcm_start") = {0};
const uint8_t test_niulai_pcm_end[1] asm("_binary_niulai_pcm_end") = {0};

const lv_font_t niulai_font_12 = {0};
const lv_font_t niulai_font_16 = {0};
const lv_font_t niulai_font_22 = {0};

static bsp_btn_cb_t s_button_callback;
static void *s_button_user;
static int s_fade_in_calls;
static int s_image_set_src_calls;
static lv_obj_t s_objects[32];
static size_t s_object_count;
static bool s_custom_voice[NIULAI_VOICE_COUNT];
static size_t s_custom_voice_length[NIULAI_VOICE_COUNT];
static lv_display_t s_display;
static lv_timer_t s_timer;
static void (*s_timer_callback)(lv_timer_t *);
static uint8_t s_backlight_percent;

static lv_obj_t *new_object(void)
{
    assert(s_object_count < sizeof(s_objects) / sizeof(s_objects[0]));
    return &s_objects[s_object_count++];
}

lv_color_t lv_color_hex(uint32_t color) { return color; }
lv_obj_t *lv_obj_create(lv_obj_t *parent) { (void)parent; return new_object(); }
lv_obj_t *lv_image_create(lv_obj_t *parent) { (void)parent; return new_object(); }
lv_obj_t *lv_label_create(lv_obj_t *parent) { (void)parent; return new_object(); }
void lv_label_set_long_mode(lv_obj_t *label, int mode) { (void)label; (void)mode; }
void lv_label_set_recolor(lv_obj_t *label, bool enabled) { label->recolor = enabled; }
void lv_label_set_text(lv_obj_t *label, const char *text)
{
    snprintf(label->text, sizeof(label->text), "%s", text);
}
void lv_label_set_text_fmt(lv_obj_t *label, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(label->text, sizeof(label->text), format, arguments);
    va_end(arguments);
}
void lv_image_set_src(lv_obj_t *image, const void *source)
{
    (void)image;
    (void)source;
    s_image_set_src_calls++;
}
void lv_obj_add_flag(lv_obj_t *object, uint32_t flag) { (void)object; (void)flag; }
void lv_obj_remove_flag(lv_obj_t *object, uint32_t flag) { (void)object; (void)flag; }
void lv_obj_center(lv_obj_t *object) { (void)object; }
void lv_obj_fade_in(lv_obj_t *object, uint32_t duration, uint32_t delay)
{
    (void)object;
    (void)duration;
    (void)delay;
    s_fade_in_calls++;
}
void lv_obj_set_pos(lv_obj_t *object, int x, int y) { object->x = x; (void)y; }
void lv_obj_set_size(lv_obj_t *object, int width, int height) { object->width = width; (void)height; }
void lv_obj_set_width(lv_obj_t *object, int width) { object->width = width; }
void lv_obj_set_style_bg_color(lv_obj_t *object, lv_color_t color, int selector) { (void)object; (void)color; (void)selector; }
void lv_obj_set_style_bg_opa(lv_obj_t *object, lv_opa_t opacity, int selector) { (void)object; (void)opacity; (void)selector; }
void lv_obj_set_style_border_color(lv_obj_t *object, lv_color_t color, int selector) { object->border_color = color; (void)selector; }
void lv_obj_set_style_border_width(lv_obj_t *object, int width, int selector) { (void)object; (void)width; (void)selector; }
void lv_obj_set_style_pad_all(lv_obj_t *object, int padding, int selector) { (void)object; (void)padding; (void)selector; }
void lv_obj_set_style_radius(lv_obj_t *object, int radius, int selector) { (void)object; (void)radius; (void)selector; }
void lv_obj_set_style_shadow_color(lv_obj_t *object, lv_color_t color, int selector) { (void)object; (void)color; (void)selector; }
void lv_obj_set_style_shadow_offset_y(lv_obj_t *object, int offset, int selector) { (void)object; (void)offset; (void)selector; }
void lv_obj_set_style_shadow_opa(lv_obj_t *object, lv_opa_t opacity, int selector) { (void)object; (void)opacity; (void)selector; }
void lv_obj_set_style_shadow_width(lv_obj_t *object, int width, int selector) { (void)object; (void)width; (void)selector; }
void lv_obj_set_style_text_align(lv_obj_t *object, lv_text_align_t align, int selector) { object->text_align = align; (void)selector; }
void lv_obj_set_style_text_color(lv_obj_t *object, lv_color_t color, int selector) { object->text_color = color; (void)selector; }
void lv_obj_set_style_text_font(lv_obj_t *object, const lv_font_t *font, int selector) { (void)object; (void)font; (void)selector; }
void lv_obj_set_style_text_line_space(lv_obj_t *object, int space, int selector) { (void)object; (void)space; (void)selector; }
void lv_screen_load(lv_obj_t *screen) { (void)screen; }
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t period,
                            void *user_data)
{
    s_timer_callback = callback;
    (void)period;
    (void)user_data;
    return &s_timer;
}

esp_err_t bsp_display_init(void) { return ESP_OK; }
lv_display_t *bsp_lvgl_init(void) { return &s_display; }
void bsp_display_backlight(uint8_t percent) { s_backlight_percent = percent; }
bool bsp_lvgl_lock(uint32_t timeout_ms) { (void)timeout_ms; return true; }
void bsp_lvgl_unlock(void) {}
esp_err_t bsp_i2c_init(void) { return ESP_OK; }
esp_err_t bsp_audio_init(void) { return ESP_FAIL; }
esp_err_t bsp_audio_set_format(uint32_t hz, uint8_t bits, uint8_t channels) { (void)hz; (void)bits; (void)channels; return ESP_OK; }
esp_err_t bsp_audio_write(const void *pcm, size_t bytes) { (void)pcm; (void)bytes; return ESP_OK; }
esp_err_t bsp_audio_read(void *pcm, size_t bytes) { (void)pcm; (void)bytes; return ESP_OK; }
void bsp_audio_set_volume(uint8_t percent) { (void)percent; }
esp_err_t bsp_battery_init(void) { return ESP_FAIL; }
int bsp_battery_soc(void) { return -1; }
esp_err_t bsp_button_init(bsp_btn_cb_t callback, void *user)
{
    s_button_callback = callback;
    s_button_user = user;
    return ESP_OK;
}

esp_err_t niulai_voice_store_init(void) { return ESP_OK; }
bool niulai_voice_store_available(void) { return false; }
bool niulai_voice_store_has(niulai_voice_slot_t slot) { return s_custom_voice[slot]; }
size_t niulai_voice_store_length(niulai_voice_slot_t slot)
{
    return s_custom_voice_length[slot];
}
esp_err_t niulai_voice_store_read(niulai_voice_slot_t slot, size_t offset,
                                  void *buffer, size_t bytes)
{
    (void)slot; (void)offset; (void)buffer; (void)bytes; return ESP_FAIL;
}
esp_err_t niulai_voice_store_begin(niulai_voice_slot_t slot) { (void)slot; return ESP_FAIL; }
esp_err_t niulai_voice_store_append(const void *pcm, size_t bytes) { (void)pcm; (void)bytes; return ESP_FAIL; }
esp_err_t niulai_voice_store_finish(void) { return ESP_FAIL; }
void niulai_voice_store_cancel(void) {}
esp_err_t niulai_voice_store_reset(void) { return ESP_FAIL; }
esp_err_t niulai_preferences_load(uint8_t *volume, uint8_t *brightness)
{
    (void)volume;
    (void)brightness;
    return ESP_OK;
}
esp_err_t niulai_preferences_save(uint8_t volume, uint8_t brightness)
{
    (void)volume;
    (void)brightness;
    return ESP_OK;
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size) { (void)length; (void)item_size; return NULL; }
BaseType_t xQueueOverwrite(QueueHandle_t queue, const void *item) { (void)queue; (void)item; return pdTRUE; }
BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t wait) { (void)queue; (void)item; (void)wait; return pdFALSE; }
void vQueueDelete(QueueHandle_t queue) { (void)queue; }
BaseType_t xTaskCreate(void (*task)(void *), const char *name,
                       uint32_t stack_depth, void *argument,
                       UBaseType_t priority, TaskHandle_t *handle)
{
    (void)task; (void)name; (void)stack_depth; (void)argument;
    (void)priority; (void)handle; return pdPASS;
}
void vTaskDelay(TickType_t ticks) { (void)ticks; }

int main(void)
{
    assert(niulai_app_start() == ESP_OK);
    assert(s_button_callback != NULL);

    lv_obj_t *title = &s_objects[3];
    lv_obj_t *phrase = &s_objects[4];
    lv_obj_t *hint = &s_objects[5];
    lv_obj_t *panel = &s_objects[2];
    lv_obj_t *reset_label = &s_objects[24];
    assert(s_backlight_percent == NIULAI_DEFAULT_BRIGHTNESS);
    assert(strcmp(title->text, "牛来") == 0);
    assert(title->text_color == 0xF7F2E8);
    assert(title->x == 20);
    assert(title->width == 200);
    assert(title->text_align == LV_TEXT_ALIGN_CENTER);
    assert(phrase->recolor);
    assert(strcmp(phrase->text,
                  "上键  #F38450 牛来#   ·   下键  #F7C161 妈妈#") == 0);

    s_button_callback(BSP_BTN_OK, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(title->text, "设置") == 0);
    assert(strcmp(hint->text,
                  "上下键  ·  确认键进入\n长按确认键返回") == 0);
    s_button_callback(BSP_BTN_DOWN, BSP_BTN_CLICK, s_button_user);
    s_button_callback(BSP_BTN_OK, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(hint->text,
                  "上键增加  ·  下键减少\n长按确认键返回") == 0);
    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    assert(s_backlight_percent == 80);
    s_button_callback(BSP_BTN_OK, BSP_BTN_CLICK, s_button_user);
    s_button_callback(BSP_BTN_DOWN, BSP_BTN_CLICK, s_button_user);
    s_button_callback(BSP_BTN_OK, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(reset_label->text, "确认恢复默认声音?") == 0);
    assert(strcmp(hint->text,
                  "确认键恢复  ·  上下键返回\n长按确认键返回") == 0);
    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(reset_label->text, "确认键恢复默认声音") == 0);
    s_button_callback(BSP_BTN_OK, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(reset_label->text, "确认恢复默认声音?") == 0);
    s_button_callback(BSP_BTN_OK, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(phrase->text, "恢复失败") == 0);
    s_button_callback(BSP_BTN_OK, BSP_BTN_LONG, s_button_user);
    assert(strcmp(title->text, "牛来") == 0);

    s_fade_in_calls = 0;
    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);

    assert(s_fade_in_calls == 0);
    assert(title->text_color == 0xF38450);
    assert(title->x == 20);
    assert(title->width == 200);
    assert(title->text_align == LV_TEXT_ALIGN_CENTER);
    assert(panel->border_color == 0xF38450);
    assert(strcmp(phrase->text, "妈妈～～") == 0);
    assert(phrase->text_color == 0xF38450);
    assert(phrase->x == 20);
    assert(phrase->width == 200);
    assert(phrase->text_align == LV_TEXT_ALIGN_CENTER);

    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(phrase->text, "妈妈！妈妈！") == 0);
    assert(panel->border_color == 0xF38450);

    s_button_callback(BSP_BTN_DOWN, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(title->text, "妈妈") == 0);
    assert(strcmp(phrase->text, "牛来！") == 0);
    assert(panel->border_color == 0xF7C161);
    assert(s_timer_callback != NULL);
    s_timer_callback(&s_timer);
    s_timer_callback(&s_timer);
    assert(panel->border_color == 0xF38450);

    s_button_callback(BSP_BTN_OK, BSP_BTN_LONG, s_button_user);
    s_button_callback(BSP_BTN_OK, BSP_BTN_DOUBLE, s_button_user);
    assert(s_fade_in_calls == 0);
    assert(strcmp(title->text, "牛来") == 0);
    assert(strcmp(phrase->text,
                  "上键  #F38450 牛来#   ·   下键  #F7C161 妈妈#") == 0);
    assert(strcmp(hint->text,
                  "声音暂不可用\n确认键进入设置") == 0);

    s_custom_voice[NIULAI_VOICE_CALF] = true;
    s_custom_voice_length[NIULAI_VOICE_CALF] = 16000;
    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(phrase->text, "@#%&*!?@#！") == 0);
    assert(phrase->text_color == 0xF38450);

    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    s_image_set_src_calls = 0;
    for (int i = 0; i < 5; ++i) s_timer_callback(&s_timer);
    int calls_after_recording = s_image_set_src_calls;
    s_timer_callback(&s_timer);
    assert(s_image_set_src_calls == calls_after_recording);

    s_button_callback(BSP_BTN_OK, BSP_BTN_LONG, s_button_user);
    s_custom_voice[NIULAI_VOICE_MOTHER] = true;
    s_button_callback(BSP_BTN_DOWN, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(title->text, "妈妈") == 0);
    assert(title->text_color == 0xF7C161);
    assert(title->text_align == LV_TEXT_ALIGN_CENTER);
    assert(panel->border_color == 0xF7C161);
    assert(strcmp(phrase->text, "#@%&*!?#@！") == 0);
    assert(phrase->text_color == 0xF7C161);

    s_button_callback(BSP_BTN_DOWN, BSP_BTN_DOUBLE, s_button_user);
    assert(strcmp(phrase->text, "#@%&*!?#@！！") == 0);

    s_button_callback(BSP_BTN_OK, BSP_BTN_LONG, s_button_user);
    s_button_callback(BSP_BTN_OK, BSP_BTN_DOUBLE, s_button_user);
    assert(strcmp(title->text, "牛来") == 0);
    assert(strcmp(phrase->text,
                  "上键  #F38450 牛来#   ·   下键  #F7C161 妈妈#") == 0);

    for (int i = 0; i < 250; ++i) s_timer_callback(&s_timer);
    assert(s_backlight_percent == 0);
    assert(strcmp(phrase->text,
                  "上键  #F38450 牛来#   ·   下键  #F7C161 妈妈#") == 0);

    s_button_callback(BSP_BTN_UP, BSP_BTN_PRESS, s_button_user);
    assert(s_backlight_percent == 80);
    assert(strcmp(phrase->text,
                  "上键  #F38450 牛来#   ·   下键  #F7C161 妈妈#") == 0);
    s_button_callback(BSP_BTN_UP, BSP_BTN_RELEASE, s_button_user);
    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(phrase->text,
                  "上键  #F38450 牛来#   ·   下键  #F7C161 妈妈#") == 0);
    s_button_callback(BSP_BTN_UP, BSP_BTN_CLICK, s_button_user);
    assert(strcmp(phrase->text, "@#%&*!?@#！") == 0);
    assert(phrase->text_color == 0xF38450);
    return 0;
}
