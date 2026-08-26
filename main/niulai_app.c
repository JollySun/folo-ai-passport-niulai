// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_app.h"

#include "niulai_audio_math.h"
#include "niulai_fonts.h"
#include "niulai_model.h"
#include "niulai_preferences.h"
#include "niulai_voice_store.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FRAME_WIDTH 240
#define HOME_HEIGHT 320
#define ACTIVE_HEIGHT 180
#define AUDIO_SAMPLE_RATE 16000
#define DEFAULT_AUDIO_SAMPLE_RATE 12000
#define AUDIO_CHUNK_SAMPLES 512
#define ANIMATION_PERIOD_MS 120
#define MAMA_ANIMATION_MS 4090
#define NIULAI_ANIMATION_MS 1350
#define BATTERY_REFRESH_MS 1000
#define ROLE_CHAIN_WINDOW_MS 1800
#define REPLY_FLASH_MS 960
#define REPLY_FLASH_STEP_MS 240
#define EXCITED_ANIMATION_EXTRA_MS 960
#define SLEEP_TIMEOUT_MS 30000
#define SLEEP_BACKLIGHT_PERCENT 0
#define SETTINGS_TRACK_WIDTH 172

#define UI_COLOR_BG 0x071A1E
#define UI_COLOR_SURFACE 0x10292D
#define UI_COLOR_SURFACE_ALT 0x18383A
#define UI_COLOR_TEXT 0xF7F2E8
#define UI_COLOR_MUTED 0xA7C2BE
#define UI_COLOR_ACCENT 0xE7A35B
#define UI_COLOR_CALF 0xF38450
#define UI_COLOR_MOTHER 0xF7C161
#define UI_COLOR_EDGE 0x3C7772
#define UI_COLOR_TRACK 0x294A4B

static const char *TAG = "niulai";

extern const uint8_t s_home_data[] asm("_binary_home_rgb565_start");
extern const uint8_t s_calf_1_data[] asm("_binary_calf_1_rgb565_start");
extern const uint8_t s_calf_2_data[] asm("_binary_calf_2_rgb565_start");
extern const uint8_t s_mother_1_data[] asm("_binary_mother_1_rgb565_start");
extern const uint8_t s_mother_2_data[] asm("_binary_mother_2_open_rgb565_start");
extern const uint8_t s_mama_pcm_start[] asm("_binary_mama_pcm_start");
extern const uint8_t s_mama_pcm_end[] asm("_binary_mama_pcm_end");
extern const uint8_t s_niulai_pcm_start[] asm("_binary_niulai_pcm_start");
extern const uint8_t s_niulai_pcm_end[] asm("_binary_niulai_pcm_end");

#define IMAGE_DSC(name, width, height, pixels)             \
    static const lv_image_dsc_t name = {                   \
        .header.magic = LV_IMAGE_HEADER_MAGIC,             \
        .header.cf = LV_COLOR_FORMAT_RGB565,               \
        .header.flags = 0,                                 \
        .header.w = (width),                               \
        .header.h = (height),                              \
        .header.stride = (width) * 2,                      \
        .data_size = (width) * (height) * 2,               \
        .data = (pixels),                                  \
    }

IMAGE_DSC(HOME_IMAGE, FRAME_WIDTH, HOME_HEIGHT, s_home_data);
IMAGE_DSC(CALF_1_IMAGE, FRAME_WIDTH, ACTIVE_HEIGHT, s_calf_1_data);
IMAGE_DSC(CALF_2_IMAGE, FRAME_WIDTH, ACTIVE_HEIGHT, s_calf_2_data);
IMAGE_DSC(MOTHER_1_IMAGE, FRAME_WIDTH, ACTIVE_HEIGHT, s_mother_1_data);
IMAGE_DSC(MOTHER_2_IMAGE, FRAME_WIDTH, ACTIVE_HEIGHT, s_mother_2_data);

typedef enum {
    AUDIO_CMD_STOP,
    AUDIO_CMD_PLAY_MAMA,
    AUDIO_CMD_PLAY_NIULAI,
    AUDIO_CMD_RECORD_CALF,
    AUDIO_CMD_RECORD_MOTHER,
    AUDIO_CMD_STOP_RECORDING,
    AUDIO_CMD_RESET_VOICES,
} audio_command_t;

typedef struct {
    uint8_t volume;
    uint8_t brightness;
} preferences_update_t;

typedef enum {
    RECORD_STATE_IDLE,
    RECORD_STATE_PREPARING,
    RECORD_STATE_ACTIVE,
    RECORD_STATE_SAVING,
    RECORD_STATE_SAVED,
    RECORD_STATE_FAILED,
    RECORD_STATE_RESET_DONE,
} record_state_t;

static niulai_model_t s_model;
static lv_obj_t *s_screen;
static lv_obj_t *s_image;
static lv_obj_t *s_panel;
static lv_obj_t *s_title;
static lv_obj_t *s_phrase;
static lv_obj_t *s_hint;
static lv_obj_t *s_battery_body;
static lv_obj_t *s_battery_fill;
static lv_obj_t *s_battery_cap;
static lv_obj_t *s_battery_value;
static lv_obj_t *s_record_dot;
static lv_obj_t *s_volume_panel;
static lv_obj_t *s_volume_label;
static lv_obj_t *s_volume_value;
static lv_obj_t *s_volume_track;
static lv_obj_t *s_volume_fill;
static lv_obj_t *s_brightness_panel;
static lv_obj_t *s_brightness_label;
static lv_obj_t *s_brightness_value;
static lv_obj_t *s_brightness_track;
static lv_obj_t *s_brightness_fill;
static lv_obj_t *s_settings_divider;
static lv_obj_t *s_voice_label;
static lv_obj_t *s_reset_panel;
static lv_obj_t *s_reset_label;
static QueueHandle_t s_audio_queue;
static QueueHandle_t s_preferences_queue;
static bool s_audio_ok;
static bool s_battery_ok;
static int s_battery_soc = -1;
static volatile uint8_t s_volume = NIULAI_DEFAULT_VOLUME;
static volatile uint8_t s_brightness = NIULAI_DEFAULT_BRIGHTNESS;
static volatile record_state_t s_record_state = RECORD_STATE_IDLE;
static volatile niulai_voice_slot_t s_record_slot = NIULAI_VOICE_CALF;
static bool s_record_button_down;
static bsp_btn_t s_record_button;
static bool s_animation_active;
static bool s_animation_frame;
static uint32_t s_animation_left_ms;
static uint32_t s_record_pulse_ms;
static bool s_record_pulse_on;
static uint32_t s_idle_ms;
static bool s_sleeping;
static bool s_ignore_wake_gesture;
static bool s_last_role_valid;
static bsp_btn_t s_last_role_button;
static uint32_t s_role_chain_ms;
static uint8_t s_emotion_level = 1;
static uint32_t s_reply_flash_ms;
static uint32_t s_reply_flash_step_ms;
static bool s_reply_flash_previous;
static uint32_t s_reply_previous_color;

static void show_current_page(void);

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font,
                            uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static void set_hidden(lv_obj_t *object, bool hidden)
{
    if (hidden) lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
}

static void set_label_layout(lv_obj_t *label, const lv_font_t *font,
                             uint32_t color, int x, int y, int width,
                             int height, lv_text_align_t align)
{
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, width, height);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, align, 0);
    lv_obj_set_style_text_line_space(label, 3, 0);
}

static void style_surface(lv_obj_t *surface, uint32_t background,
                          uint32_t border, lv_opa_t opacity, int radius,
                          int border_width)
{
    lv_obj_set_style_bg_color(surface, lv_color_hex(background), 0);
    lv_obj_set_style_bg_opa(surface, opacity, 0);
    lv_obj_set_style_border_color(surface, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(surface, border_width, 0);
    lv_obj_set_style_radius(surface, radius, 0);
}

static void set_settings_widgets_hidden(bool hidden)
{
    set_hidden(s_volume_panel, hidden);
    set_hidden(s_volume_label, hidden);
    set_hidden(s_volume_value, hidden);
    set_hidden(s_volume_track, hidden);
    set_hidden(s_brightness_panel, hidden);
    set_hidden(s_brightness_label, hidden);
    set_hidden(s_brightness_value, hidden);
    set_hidden(s_brightness_track, hidden);
    set_hidden(s_settings_divider, hidden);
    set_hidden(s_voice_label, hidden);
    set_hidden(s_reset_panel, hidden);
}

static void set_primary_widgets_hidden(bool hidden)
{
    set_hidden(s_panel, hidden);
    set_hidden(s_title, hidden);
    set_hidden(s_phrase, hidden);
    set_hidden(s_hint, hidden);
}

static const lv_image_dsc_t *animation_image(bool second_frame)
{
    bool calf = s_model.page == NIULAI_PAGE_CALF;
    if (calf) {
        return second_frame ? &CALF_2_IMAGE : &CALF_1_IMAGE;
    }
    return second_frame ? &MOTHER_2_IMAGE : &MOTHER_1_IMAGE;
}

static void set_battery_hidden(bool hidden)
{
    set_hidden(s_battery_body, hidden);
    set_hidden(s_battery_cap, hidden);
}

static void update_battery_icon(void)
{
    if (!s_battery_body) return;

    int percent = s_battery_soc;
    if (percent < 0) {
        lv_obj_set_width(s_battery_fill, 0);
        lv_label_set_text(s_battery_value, "");
        lv_obj_set_style_border_color(s_battery_body, lv_color_hex(UI_COLOR_EDGE), 0);
        lv_obj_set_style_bg_opa(s_battery_body, LV_OPA_60, 0);
        return;
    }
    if (percent > 100) percent = 100;

    int fill_width = (percent * 34 + 99) / 100;
    lv_obj_set_width(s_battery_fill, fill_width);
    lv_obj_set_style_bg_color(s_battery_fill, lv_color_hex(UI_COLOR_ACCENT), 0);
    lv_obj_set_style_border_color(s_battery_body, lv_color_hex(UI_COLOR_EDGE), 0);
    lv_obj_set_style_bg_opa(s_battery_body, LV_OPA_80, 0);
    lv_label_set_text_fmt(s_battery_value, "%d", percent);
}

static uint32_t role_color(bsp_btn_t button)
{
    return button == BSP_BTN_UP ? UI_COLOR_CALF : UI_COLOR_MOTHER;
}

static void clear_role_interaction(void)
{
    s_last_role_valid = false;
    s_role_chain_ms = 0;
    s_emotion_level = 1;
    s_reply_flash_ms = 0;
    s_reply_flash_step_ms = 0;
    s_reply_flash_previous = false;
}

static void wake_display(void)
{
    s_sleeping = false;
    s_idle_ms = 0;
    bsp_display_backlight(s_brightness);
    if (bsp_lvgl_lock(500)) {
        show_current_page();
        bsp_lvgl_unlock();
    }
}

static void show_home(void)
{
    s_animation_active = false;
    set_primary_widgets_hidden(false);
    set_hidden(s_image, false);
    set_hidden(s_record_dot, true);
    set_settings_widgets_hidden(true);
    set_battery_hidden(false);
    lv_obj_set_pos(s_image, 0, 0);
    lv_image_set_src(s_image, &HOME_IMAGE);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(UI_COLOR_BG), 0);

    lv_obj_set_pos(s_panel, 8, 188);
    lv_obj_set_size(s_panel, 224, 124);
    style_surface(s_panel, UI_COLOR_SURFACE, UI_COLOR_EDGE, LV_OPA_90, 16, 1);

    set_label_layout(s_title, &niulai_font_22, UI_COLOR_TEXT,
                     20, 197, 200, 28, LV_TEXT_ALIGN_CENTER);
    set_label_layout(s_phrase, &niulai_font_16, UI_COLOR_TEXT,
                     20, 229, 200, 23, LV_TEXT_ALIGN_LEFT);
    set_label_layout(s_hint, &niulai_font_12, UI_COLOR_MUTED,
                     20, 260, 200, 42, LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(s_title, "牛来");
    lv_label_set_recolor(s_phrase, true);
    lv_label_set_text_fmt(s_phrase,
        "上键  #%06lX 牛来#   ·   下键  #%06lX 妈妈#",
        (unsigned long)UI_COLOR_CALF, (unsigned long)UI_COLOR_MOTHER);
    lv_label_set_text(s_hint, s_audio_ok
        ? "长按对应按键可录音\n确认键进入设置"
        : "声音暂不可用\n确认键进入设置");
    update_battery_icon();
}

static void show_active_page(void)
{
    bool calf = s_model.page == NIULAI_PAGE_CALF;
    uint32_t page_color = calf ? UI_COLOR_CALF : UI_COLOR_MOTHER;
    niulai_voice_slot_t voice_slot = calf ? NIULAI_VOICE_CALF
                                          : NIULAI_VOICE_MOTHER;
    bool custom_voice = niulai_voice_store_has(voice_slot);
    bool record_page = (calf && s_record_slot == NIULAI_VOICE_CALF) ||
                       (!calf && s_record_slot == NIULAI_VOICE_MOTHER);
    set_primary_widgets_hidden(false);
    set_hidden(s_image, false);
    set_settings_widgets_hidden(true);
    set_battery_hidden(true);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_pos(s_image, 0, 42);
    lv_image_set_src(s_image, animation_image(false));

    lv_obj_set_pos(s_panel, 8, 224);
    lv_obj_set_size(s_panel, 224, 88);
    style_surface(s_panel, UI_COLOR_SURFACE, page_color,
                  LV_OPA_COVER, 16, 1);

    set_label_layout(s_title, &niulai_font_22, page_color,
                     20, 10, 200, 28, LV_TEXT_ALIGN_CENTER);
    set_label_layout(s_phrase, &niulai_font_22, UI_COLOR_TEXT,
                     20, 241, 200, 32, LV_TEXT_ALIGN_CENTER);
    set_label_layout(s_hint, &niulai_font_12, UI_COLOR_MUTED,
                     20, 286, 200, 18, LV_TEXT_ALIGN_CENTER);
    lv_label_set_recolor(s_phrase, false);
    lv_label_set_text(s_title, calf ? "牛来" : "妈妈");
    if (record_page && s_record_state == RECORD_STATE_PREPARING) {
        lv_label_set_text(s_phrase, "准备录音");
    } else if (record_page && s_record_state == RECORD_STATE_ACTIVE) {
        lv_label_set_text(s_phrase, "正在录音");
    } else if (record_page && s_record_state == RECORD_STATE_SAVING) {
        lv_label_set_text(s_phrase, "正在保存");
    } else if (record_page && s_record_state == RECORD_STATE_SAVED) {
        lv_label_set_text(s_phrase, "录音已保存");
    } else if (record_page && s_record_state == RECORD_STATE_FAILED) {
        lv_label_set_text(s_phrase, "录音失败");
    } else if (custom_voice && s_emotion_level >= 2) {
        lv_obj_set_style_text_color(s_phrase, lv_color_hex(page_color), 0);
        lv_label_set_text(s_phrase,
                          calf ? "@#%&*!?@#！！" : "#@%&*!?#@！！");
    } else if (custom_voice) {
        lv_obj_set_style_text_color(s_phrase, lv_color_hex(page_color), 0);
        lv_label_set_text(s_phrase,
                          calf ? "@#%&*!?@#！" : "#@%&*!?#@！");
    } else if (s_emotion_level >= 2) {
        lv_obj_set_style_text_color(s_phrase, lv_color_hex(page_color), 0);
        lv_label_set_text(s_phrase, calf ? "妈妈！妈妈！" : "牛来！牛来！");
    } else {
        lv_obj_set_style_text_color(s_phrase, lv_color_hex(page_color), 0);
        lv_label_set_text(s_phrase, calf ? "妈妈～～" : "牛来！");
    }
    lv_label_set_text(s_hint,
        record_page && (s_record_state == RECORD_STATE_PREPARING ||
                        s_record_state == RECORD_STATE_ACTIVE)
            ? "松开按键保存" : "长按确认键返回首页");

    bool recording = record_page &&
                     (s_record_state == RECORD_STATE_PREPARING ||
                      s_record_state == RECORD_STATE_ACTIVE ||
                      s_record_state == RECORD_STATE_SAVING);
    set_hidden(s_record_dot, !recording);
    if (recording) {
        lv_obj_set_style_bg_opa(s_record_dot, LV_OPA_COVER, 0);
        s_record_pulse_ms = 0;
        s_record_pulse_on = true;
    }
}

static void show_settings(void)
{
    s_animation_active = false;
    set_primary_widgets_hidden(false);
    set_hidden(s_image, true);
    set_hidden(s_record_dot, true);
    set_settings_widgets_hidden(false);
    set_battery_hidden(true);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(UI_COLOR_BG), 0);

    lv_obj_set_pos(s_panel, 12, 50);
    lv_obj_set_size(s_panel, 216, 246);
    style_surface(s_panel, UI_COLOR_SURFACE, UI_COLOR_EDGE,
                  LV_OPA_COVER, 18, 1);

    set_label_layout(s_title, &niulai_font_22, UI_COLOR_TEXT,
                     18, 14, 204, 30, LV_TEXT_ALIGN_LEFT);
    lv_label_set_recolor(s_phrase, false);
    set_label_layout(s_phrase, &niulai_font_12, UI_COLOR_TEXT,
                     116, 198, 96, 18, LV_TEXT_ALIGN_RIGHT);
    set_label_layout(s_hint, &niulai_font_12, UI_COLOR_MUTED,
                     20, 260, 200, 36, LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(s_title, "设置");

    bool volume_selected = s_model.setting == NIULAI_SETTING_VOLUME;
    bool brightness_selected = s_model.setting == NIULAI_SETTING_BRIGHTNESS;
    bool reset_selected = s_model.setting == NIULAI_SETTING_RESET_VOICES;
    style_surface(s_volume_panel, UI_COLOR_SURFACE_ALT,
                  volume_selected ? UI_COLOR_ACCENT : UI_COLOR_EDGE,
                  volume_selected ? LV_OPA_COVER : LV_OPA_TRANSP,
                  10, volume_selected ? (s_model.setting_active ? 2 : 1) : 1);
    style_surface(s_brightness_panel, UI_COLOR_SURFACE_ALT,
                  brightness_selected ? UI_COLOR_ACCENT : UI_COLOR_EDGE,
                  brightness_selected ? LV_OPA_COVER : LV_OPA_TRANSP,
                  10, brightness_selected ? (s_model.setting_active ? 2 : 1) : 1);

    lv_label_set_text(s_volume_label, "音量");
    lv_label_set_text_fmt(s_volume_value, "%d%%", s_model.volume);
    lv_obj_set_style_text_color(s_volume_label,
        lv_color_hex(volume_selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT), 0);
    lv_obj_set_style_text_color(s_volume_value,
        lv_color_hex(volume_selected ? UI_COLOR_ACCENT : UI_COLOR_MUTED), 0);
    int target_width =
        (s_model.volume * SETTINGS_TRACK_WIDTH + 99) / 100;
    lv_obj_set_width(s_volume_fill, target_width);
    lv_obj_set_style_bg_color(s_volume_fill,
        lv_color_hex(volume_selected ? UI_COLOR_ACCENT : UI_COLOR_EDGE), 0);

    lv_label_set_text(s_brightness_label, "亮度");
    lv_label_set_text_fmt(s_brightness_value, "%d%%", s_model.brightness);
    lv_obj_set_style_text_color(s_brightness_label,
        lv_color_hex(brightness_selected ? UI_COLOR_ACCENT : UI_COLOR_TEXT), 0);
    lv_obj_set_style_text_color(s_brightness_value,
        lv_color_hex(brightness_selected ? UI_COLOR_ACCENT : UI_COLOR_MUTED), 0);
    target_width = (s_model.brightness * SETTINGS_TRACK_WIDTH + 99) / 100;
    lv_obj_set_width(s_brightness_fill, target_width);
    lv_obj_set_style_bg_color(s_brightness_fill,
        lv_color_hex(brightness_selected ? UI_COLOR_ACCENT : UI_COLOR_EDGE), 0);

    lv_label_set_text(s_voice_label, "自定义声音");

    int custom_count = (niulai_voice_store_has(NIULAI_VOICE_CALF) ? 1 : 0) +
                       (niulai_voice_store_has(NIULAI_VOICE_MOTHER) ? 1 : 0);
    if (s_record_state == RECORD_STATE_PREPARING) {
        lv_label_set_text(s_phrase, "正在恢复");
    } else if (s_record_state == RECORD_STATE_FAILED) {
        lv_label_set_text(s_phrase, "恢复失败");
    } else if (!niulai_voice_store_available()) {
        lv_label_set_text(s_phrase, "录音存储不可用");
    } else if (s_record_state == RECORD_STATE_RESET_DONE) {
        lv_label_set_text(s_phrase, "已恢复默认声音");
    } else if (custom_count > 0) {
        lv_label_set_text_fmt(s_phrase, "已保存 %d 个", custom_count);
    } else {
        lv_label_set_text(s_phrase, "使用默认声音");
    }
    style_surface(s_reset_panel, UI_COLOR_SURFACE_ALT,
                  s_model.reset_confirming ? UI_COLOR_MOTHER
                                           : (reset_selected ? UI_COLOR_ACCENT
                                                             : UI_COLOR_EDGE),
                  LV_OPA_COVER, 9,
                  reset_selected ? (s_model.reset_confirming ? 2 : 1) : 1);
    lv_obj_set_style_text_color(s_reset_label,
        lv_color_hex(reset_selected ? UI_COLOR_TEXT : UI_COLOR_MUTED), 0);
    lv_label_set_text(s_reset_label, s_model.reset_confirming
        ? "确认恢复默认声音?" : "确认键恢复默认声音");

    if (s_model.reset_confirming) {
        lv_label_set_text(s_hint,
                          "确认键恢复  ·  上下键返回\n长按确认键返回");
    } else if (s_model.setting_active) {
        lv_label_set_text(s_hint,
                          "上键增加  ·  下键减少\n长按确认键返回");
    } else {
        lv_label_set_text(s_hint,
                          "上下键  ·  确认键进入\n长按确认键返回");
    }
}

static void show_current_page(void)
{
    if (s_model.page == NIULAI_PAGE_HOME) show_home();
    else if (s_model.page == NIULAI_PAGE_SETTINGS) show_settings();
    else show_active_page();
}

static uint32_t voice_animation_duration_ms(niulai_action_t action,
                                            bool *custom_voice)
{
    bool calf_voice = action == NIULAI_ACTION_PLAY_MAMA;
    niulai_voice_slot_t slot = calf_voice ? NIULAI_VOICE_CALF
                                          : NIULAI_VOICE_MOTHER;
    *custom_voice = niulai_voice_store_has(slot);
    if (!*custom_voice) {
        return calf_voice ? MAMA_ANIMATION_MS : NIULAI_ANIMATION_MS;
    }

    size_t bytes_per_second = NIULAI_RECORD_SAMPLE_RATE * sizeof(int16_t);
    size_t bytes = niulai_voice_store_length(slot);
    return (uint32_t)((bytes * 1000U + bytes_per_second - 1) /
                      bytes_per_second);
}

static void start_animation(uint32_t duration_ms)
{
    s_animation_active = true;
    s_animation_frame = false;
    s_animation_left_ms = duration_ms;
    lv_image_set_src(s_image, animation_image(false));
}

static void animation_tick(lv_timer_t *timer)
{
    (void)timer;
    bool recording = s_record_state == RECORD_STATE_PREPARING ||
                     s_record_state == RECORD_STATE_ACTIVE ||
                     s_record_state == RECORD_STATE_SAVING;
    if (recording) {
        s_idle_ms = 0;
    } else if (!s_sleeping) {
        s_idle_ms += ANIMATION_PERIOD_MS;
        if (s_idle_ms >= SLEEP_TIMEOUT_MS) {
            s_sleeping = true;
            s_animation_active = false;
            clear_role_interaction();
            bsp_display_backlight(SLEEP_BACKLIGHT_PERCENT);
        }
    }

    if (s_role_chain_ms > 0) {
        s_role_chain_ms = s_role_chain_ms <= ANIMATION_PERIOD_MS
                        ? 0 : s_role_chain_ms - ANIMATION_PERIOD_MS;
        if (s_role_chain_ms == 0) s_last_role_valid = false;
    }

    if (s_reply_flash_ms > 0 &&
        (s_model.page == NIULAI_PAGE_CALF ||
         s_model.page == NIULAI_PAGE_MOTHER)) {
        s_reply_flash_step_ms += ANIMATION_PERIOD_MS;
        if (s_reply_flash_step_ms >= REPLY_FLASH_STEP_MS) {
            s_reply_flash_step_ms = 0;
            s_reply_flash_previous = !s_reply_flash_previous;
            uint32_t current_color = s_model.page == NIULAI_PAGE_CALF
                                   ? UI_COLOR_CALF : UI_COLOR_MOTHER;
            lv_obj_set_style_border_color(s_panel,
                lv_color_hex(s_reply_flash_previous
                             ? s_reply_previous_color : current_color), 0);
        }
        if (s_reply_flash_ms <= ANIMATION_PERIOD_MS) {
            s_reply_flash_ms = 0;
            s_reply_flash_previous = false;
            uint32_t current_color = s_model.page == NIULAI_PAGE_CALF
                                   ? UI_COLOR_CALF : UI_COLOR_MOTHER;
            lv_obj_set_style_border_color(s_panel, lv_color_hex(current_color), 0);
        } else {
            s_reply_flash_ms -= ANIMATION_PERIOD_MS;
        }
    }

    bool pulse_recording = s_record_state == RECORD_STATE_ACTIVE &&
        (s_model.page == NIULAI_PAGE_CALF ||
         s_model.page == NIULAI_PAGE_MOTHER);
    if (pulse_recording) {
        s_record_pulse_ms += ANIMATION_PERIOD_MS;
        if (s_record_pulse_ms >= 480) {
            s_record_pulse_ms = 0;
            s_record_pulse_on = !s_record_pulse_on;
            lv_obj_set_style_bg_opa(s_record_dot,
                s_record_pulse_on ? LV_OPA_COVER : LV_OPA_30, 0);
        }
    }

    if (!s_animation_active || s_model.page == NIULAI_PAGE_HOME ||
        s_model.page == NIULAI_PAGE_SETTINGS) return;

    s_animation_frame = !s_animation_frame;
    lv_image_set_src(s_image, animation_image(s_animation_frame));
    if (s_animation_left_ms <= ANIMATION_PERIOD_MS) {
        s_animation_active = false;
        s_animation_frame = false;
        lv_image_set_src(s_image, animation_image(false));
    } else {
        s_animation_left_ms -= ANIMATION_PERIOD_MS;
    }
}

static void audio_request(audio_command_t command)
{
    if (s_audio_queue) xQueueOverwrite(s_audio_queue, &command);
}

static void preferences_request_save(void)
{
    if (!s_preferences_queue) return;
    preferences_update_t update = {
        .volume = s_model.volume,
        .brightness = s_model.brightness,
    };
    xQueueOverwrite(s_preferences_queue, &update);
}

static void preferences_task(void *arg)
{
    (void)arg;
    preferences_update_t update;
    for (;;) {
        if (xQueueReceive(s_preferences_queue, &update, portMAX_DELAY) == pdTRUE) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(
                niulai_preferences_save(update.volume, update.brightness));
        }
    }
}

static void set_record_state(record_state_t state)
{
    s_record_state = state;
    if (bsp_lvgl_lock(500)) {
        show_current_page();
        bsp_lvgl_unlock();
    }
}

static audio_command_t play_voice(audio_command_t command)
{
    bool calf_voice = command == AUDIO_CMD_PLAY_MAMA;
    niulai_voice_slot_t slot = calf_voice ? NIULAI_VOICE_CALF : NIULAI_VOICE_MOTHER;
    bool custom = niulai_voice_store_has(slot);
    const uint8_t *default_data = calf_voice ? s_mama_pcm_start : s_niulai_pcm_start;
    const uint8_t *default_end = calf_voice ? s_mama_pcm_end : s_niulai_pcm_end;
    size_t total = custom ? niulai_voice_store_length(slot)
                          : (size_t)(default_end - default_data);
    uint32_t sample_rate = custom ? AUDIO_SAMPLE_RATE
                                  : DEFAULT_AUDIO_SAMPLE_RATE;

    if (bsp_audio_set_format(sample_rate, 16, 1) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure audio format");
        return AUDIO_CMD_STOP;
    }
    bsp_audio_set_volume(100);
    int16_t raw[AUDIO_CHUNK_SAMPLES];
    int16_t scaled[AUDIO_CHUNK_SAMPLES];

    for (size_t offset = 0; offset < total;) {
        size_t remaining = total - offset;
        size_t bytes = remaining < sizeof(raw) ? remaining : sizeof(raw);
        if (custom) {
            if (niulai_voice_store_read(slot, offset, raw, bytes) != ESP_OK) {
                ESP_LOGE(TAG, "Custom voice read failed");
                return AUDIO_CMD_STOP;
            }
        } else {
            memcpy(raw, default_data + offset, bytes);
        }
        niulai_scale_pcm16(scaled, raw, bytes / 2, s_volume);
        if (bsp_audio_write(scaled, bytes) != ESP_OK) {
            ESP_LOGE(TAG, "Audio playback failed");
            return AUDIO_CMD_STOP;
        }
        offset += bytes;

        audio_command_t next;
        if (xQueueReceive(s_audio_queue, &next, 0) == pdTRUE) {
            return next;
        }
    }
    return AUDIO_CMD_STOP;
}

static audio_command_t record_voice(audio_command_t command)
{
    niulai_voice_slot_t slot = command == AUDIO_CMD_RECORD_CALF
                             ? NIULAI_VOICE_CALF : NIULAI_VOICE_MOTHER;
    s_record_slot = slot;
    set_record_state(RECORD_STATE_PREPARING);

    if (niulai_voice_store_begin(slot) != ESP_OK ||
        bsp_audio_set_format(NIULAI_RECORD_SAMPLE_RATE, 16, 1) != ESP_OK) {
        niulai_voice_store_cancel();
        s_record_button_down = false;
        set_record_state(RECORD_STATE_FAILED);
        return AUDIO_CMD_STOP;
    }

    set_record_state(RECORD_STATE_ACTIVE);
    int16_t pcm[AUDIO_CHUNK_SAMPLES];
    size_t total = 0;
    audio_command_t next = AUDIO_CMD_STOP;
    bool failed = false;

    while (total < NIULAI_RECORD_MAX_BYTES) {
        size_t bytes = NIULAI_RECORD_MAX_BYTES - total;
        if (bytes > sizeof(pcm)) bytes = sizeof(pcm);
        if (bsp_audio_read(pcm, bytes) != ESP_OK ||
            niulai_voice_store_append(pcm, bytes) != ESP_OK) {
            failed = true;
            break;
        }
        total += bytes;
        if (xQueueReceive(s_audio_queue, &next, 0) == pdTRUE) break;
    }

    set_record_state(RECORD_STATE_SAVING);
    if (failed || niulai_voice_store_finish() != ESP_OK) {
        niulai_voice_store_cancel();
        s_record_button_down = false;
        set_record_state(RECORD_STATE_FAILED);
    } else {
        s_record_button_down = false;
        set_record_state(RECORD_STATE_SAVED);
    }
    return next == AUDIO_CMD_STOP_RECORDING ? AUDIO_CMD_STOP : next;
}

static void audio_task(void *arg)
{
    (void)arg;
    audio_command_t command;
    for (;;) {
        if (xQueueReceive(s_audio_queue, &command, portMAX_DELAY) != pdTRUE) continue;

        while (command != AUDIO_CMD_STOP) {
            if (command == AUDIO_CMD_PLAY_MAMA || command == AUDIO_CMD_PLAY_NIULAI) {
                command = play_voice(command);
            } else if (command == AUDIO_CMD_RECORD_CALF ||
                       command == AUDIO_CMD_RECORD_MOTHER) {
                command = record_voice(command);
            } else if (command == AUDIO_CMD_RESET_VOICES) {
                set_record_state(niulai_voice_store_reset() == ESP_OK
                                 ? RECORD_STATE_RESET_DONE : RECORD_STATE_FAILED);
                command = AUDIO_CMD_STOP;
            } else {
                command = AUDIO_CMD_STOP;
            }
        }
    }
}

static niulai_input_t map_input(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (event == BSP_BTN_CLICK && button == BSP_BTN_UP) return NIULAI_INPUT_UP_CLICK;
    if (event == BSP_BTN_CLICK && button == BSP_BTN_DOWN) return NIULAI_INPUT_DOWN_CLICK;
    if (event == BSP_BTN_CLICK && button == BSP_BTN_OK) return NIULAI_INPUT_OK_CLICK;
    if (event == BSP_BTN_LONG && button == BSP_BTN_OK) return NIULAI_INPUT_OK_LONG;
    return NIULAI_INPUT_NONE;
}

static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;

    if (s_sleeping) {
        wake_display();
        s_ignore_wake_gesture = event == BSP_BTN_PRESS;
        return;
    }
    if (s_ignore_wake_gesture) {
        if (event == BSP_BTN_CLICK || event == BSP_BTN_DOUBLE ||
            event == BSP_BTN_LONG) {
            s_ignore_wake_gesture = false;
        }
        return;
    }
    s_idle_ms = 0;

    if (event == BSP_BTN_RELEASE && s_record_button_down && button == s_record_button) {
        s_record_button_down = false;
        s_record_state = RECORD_STATE_SAVING;
        if (bsp_lvgl_lock(500)) {
            show_current_page();
            bsp_lvgl_unlock();
        }
        audio_request(AUDIO_CMD_STOP_RECORDING);
        return;
    }

    bool record_calf = event == BSP_BTN_LONG && button == BSP_BTN_UP &&
                       s_model.page == NIULAI_PAGE_CALF;
    bool record_mother = event == BSP_BTN_LONG && button == BSP_BTN_DOWN &&
                         s_model.page == NIULAI_PAGE_MOTHER;
    if (record_calf || record_mother) {
        clear_role_interaction();
        s_record_slot = record_calf ? NIULAI_VOICE_CALF : NIULAI_VOICE_MOTHER;
        s_record_button = button;
        s_record_state = (s_audio_ok && niulai_voice_store_available())
                       ? RECORD_STATE_PREPARING : RECORD_STATE_FAILED;
        s_record_button_down = s_record_state == RECORD_STATE_PREPARING;
        if (bsp_lvgl_lock(500)) {
            show_current_page();
            bsp_lvgl_unlock();
        }
        if (s_record_state == RECORD_STATE_PREPARING) {
            audio_request(record_calf ? AUDIO_CMD_RECORD_CALF
                                      : AUDIO_CMD_RECORD_MOTHER);
        }
        return;
    }

    bool role_activation =
        (event == BSP_BTN_CLICK || event == BSP_BTN_DOUBLE) &&
        (button == BSP_BTN_UP || button == BSP_BTN_DOWN) &&
        s_model.page != NIULAI_PAGE_SETTINGS;
    niulai_input_t input = role_activation
                         ? (button == BSP_BTN_UP ? NIULAI_INPUT_UP_CLICK
                                                : NIULAI_INPUT_DOWN_CLICK)
                         : map_input(button, event);
    if (input == NIULAI_INPUT_NONE) return;

    if (role_activation) {
        bool chained = s_last_role_valid && s_role_chain_ms > 0;
        bool same_role = chained && s_last_role_button == button;
        bool reply = chained && s_last_role_button != button;
        s_emotion_level = (event == BSP_BTN_DOUBLE || same_role) ? 2 : 1;
        if (reply) {
            s_reply_flash_ms = REPLY_FLASH_MS;
            s_reply_flash_step_ms = 0;
            s_reply_flash_previous = false;
            s_reply_previous_color = role_color(s_last_role_button);
        } else {
            s_reply_flash_ms = 0;
        }
        s_last_role_button = button;
        s_last_role_valid = true;
        s_role_chain_ms = ROLE_CHAIN_WINDOW_MS;
    } else {
        clear_role_interaction();
    }

    if (s_record_state == RECORD_STATE_SAVED ||
        s_record_state == RECORD_STATE_FAILED ||
        s_record_state == RECORD_STATE_RESET_DONE) {
        s_record_state = RECORD_STATE_IDLE;
    }

    if (!bsp_lvgl_lock(500)) return;
    niulai_action_t action = niulai_model_apply(&s_model, input);

    if (action == NIULAI_ACTION_RESET_VOICES) {
        s_record_state = (s_audio_queue && niulai_voice_store_available())
                       ? RECORD_STATE_PREPARING : RECORD_STATE_FAILED;
    }

    show_current_page();
    if (action == NIULAI_ACTION_PLAY_MAMA || action == NIULAI_ACTION_PLAY_NIULAI) {
        bool custom_voice;
        uint32_t duration = voice_animation_duration_ms(action, &custom_voice);
        if (!custom_voice && s_emotion_level >= 2) {
            duration += EXCITED_ANIMATION_EXTRA_MS;
        }
        start_animation(duration);
    }
    bsp_lvgl_unlock();

    if (action == NIULAI_ACTION_PLAY_MAMA) audio_request(AUDIO_CMD_PLAY_MAMA);
    else if (action == NIULAI_ACTION_PLAY_NIULAI) audio_request(AUDIO_CMD_PLAY_NIULAI);
    else if (action == NIULAI_ACTION_STOP_AUDIO) audio_request(AUDIO_CMD_STOP);
    else if (action == NIULAI_ACTION_VOLUME_CHANGED) {
        s_volume = s_model.volume;
        preferences_request_save();
    } else if (action == NIULAI_ACTION_BRIGHTNESS_CHANGED) {
        s_brightness = s_model.brightness;
        bsp_display_backlight(s_brightness);
        preferences_request_save();
    } else if (action == NIULAI_ACTION_RESET_VOICES &&
               s_record_state == RECORD_STATE_PREPARING) {
        audio_request(AUDIO_CMD_RESET_VOICES);
    }
}

static void battery_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (!s_battery_ok) s_battery_ok = bsp_battery_init() == ESP_OK;
        if (s_battery_ok) {
            int soc = bsp_battery_soc();
            if (soc >= 0) s_battery_soc = soc;
        }
        if (bsp_lvgl_lock(500)) {
            update_battery_icon();
            bsp_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(BATTERY_REFRESH_MS));
    }
}

static void build_ui(void)
{
    s_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(UI_COLOR_BG), 0);

    s_image = lv_image_create(s_screen);
    s_panel = lv_obj_create(s_screen);
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_panel, 0, 0);
    lv_obj_set_style_shadow_color(s_panel, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_style_shadow_opa(s_panel, LV_OPA_40, 0);
    lv_obj_set_style_shadow_width(s_panel, 10, 0);
    lv_obj_set_style_shadow_offset_y(s_panel, 3, 0);

    s_title = make_label(s_screen, &niulai_font_22, UI_COLOR_TEXT);
    s_phrase = make_label(s_screen, &niulai_font_16, UI_COLOR_TEXT);
    s_hint = make_label(s_screen, &niulai_font_12, UI_COLOR_MUTED);

    // 紧凑的电池图标:数字置于圆角电池内部,不显示额外文字前缀。
    s_battery_body = lv_obj_create(s_screen);
    lv_obj_set_pos(s_battery_body, 10, 10);
    lv_obj_set_size(s_battery_body, 40, 18);
    lv_obj_remove_flag(s_battery_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_battery_body, 0, 0);
    lv_obj_set_style_radius(s_battery_body, 6, 0);
    lv_obj_set_style_border_width(s_battery_body, 1, 0);
    lv_obj_set_style_border_color(s_battery_body, lv_color_hex(UI_COLOR_EDGE), 0);
    lv_obj_set_style_bg_color(s_battery_body, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_battery_body, LV_OPA_80, 0);

    s_battery_fill = lv_obj_create(s_battery_body);
    lv_obj_set_pos(s_battery_fill, 2, 2);
    lv_obj_set_size(s_battery_fill, 0, 12);
    lv_obj_remove_flag(s_battery_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_battery_fill, 0, 0);
    lv_obj_set_style_border_width(s_battery_fill, 0, 0);
    lv_obj_set_style_radius(s_battery_fill, 4, 0);
    lv_obj_set_style_bg_color(s_battery_fill, lv_color_hex(UI_COLOR_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_battery_fill, LV_OPA_COVER, 0);

    s_battery_value = lv_label_create(s_battery_body);
    lv_obj_set_width(s_battery_value, 38);
    lv_obj_set_style_text_align(s_battery_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_battery_value, &niulai_font_12, 0);
    lv_obj_set_style_text_color(s_battery_value, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_set_style_bg_opa(s_battery_value, LV_OPA_TRANSP, 0);
    lv_obj_center(s_battery_value);

    s_battery_cap = lv_obj_create(s_screen);
    lv_obj_set_pos(s_battery_cap, 51, 15);
    lv_obj_set_size(s_battery_cap, 3, 8);
    lv_obj_set_style_pad_all(s_battery_cap, 0, 0);
    lv_obj_set_style_border_width(s_battery_cap, 0, 0);
    lv_obj_set_style_radius(s_battery_cap, 2, 0);
    lv_obj_set_style_bg_color(s_battery_cap, lv_color_hex(UI_COLOR_EDGE), 0);
    lv_obj_set_style_bg_opa(s_battery_cap, LV_OPA_COVER, 0);

    s_record_dot = lv_obj_create(s_screen);
    lv_obj_set_pos(s_record_dot, 214, 18);
    lv_obj_set_size(s_record_dot, 8, 8);
    lv_obj_remove_flag(s_record_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_record_dot, 0, 0);
    lv_obj_set_style_border_width(s_record_dot, 0, 0);
    lv_obj_set_style_radius(s_record_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_record_dot, lv_color_hex(UI_COLOR_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_record_dot, LV_OPA_COVER, 0);

    s_volume_panel = lv_obj_create(s_screen);
    lv_obj_set_pos(s_volume_panel, 22, 59);
    lv_obj_set_size(s_volume_panel, 196, 62);
    lv_obj_remove_flag(s_volume_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_volume_panel, 0, 0);

    s_volume_label = make_label(s_screen, &niulai_font_16, UI_COLOR_TEXT);
    set_label_layout(s_volume_label, &niulai_font_16, UI_COLOR_TEXT,
                     30, 66, 100, 24, LV_TEXT_ALIGN_LEFT);
    s_volume_value = make_label(s_screen, &niulai_font_16, UI_COLOR_ACCENT);
    set_label_layout(s_volume_value, &niulai_font_16, UI_COLOR_ACCENT,
                     158, 66, 52, 24, LV_TEXT_ALIGN_RIGHT);

    s_volume_track = lv_obj_create(s_screen);
    lv_obj_set_pos(s_volume_track, 34, 99);
    lv_obj_set_size(s_volume_track, SETTINGS_TRACK_WIDTH, 8);
    lv_obj_remove_flag(s_volume_track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_volume_track, 0, 0);
    lv_obj_set_style_border_width(s_volume_track, 0, 0);
    lv_obj_set_style_radius(s_volume_track, 4, 0);
    lv_obj_set_style_bg_color(s_volume_track, lv_color_hex(UI_COLOR_TRACK), 0);
    lv_obj_set_style_bg_opa(s_volume_track, LV_OPA_COVER, 0);

    s_volume_fill = lv_obj_create(s_volume_track);
    lv_obj_set_pos(s_volume_fill, 0, 0);
    lv_obj_set_size(s_volume_fill, 0, 8);
    lv_obj_remove_flag(s_volume_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_volume_fill, 0, 0);
    lv_obj_set_style_border_width(s_volume_fill, 0, 0);
    lv_obj_set_style_radius(s_volume_fill, 4, 0);
    lv_obj_set_style_bg_color(s_volume_fill, lv_color_hex(UI_COLOR_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_volume_fill, LV_OPA_COVER, 0);

    s_brightness_panel = lv_obj_create(s_screen);
    lv_obj_set_pos(s_brightness_panel, 22, 127);
    lv_obj_set_size(s_brightness_panel, 196, 62);
    lv_obj_remove_flag(s_brightness_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_brightness_panel, 0, 0);

    s_brightness_label = make_label(s_screen, &niulai_font_16, UI_COLOR_TEXT);
    set_label_layout(s_brightness_label, &niulai_font_16, UI_COLOR_TEXT,
                     30, 134, 100, 24, LV_TEXT_ALIGN_LEFT);
    s_brightness_value = make_label(s_screen, &niulai_font_16, UI_COLOR_ACCENT);
    set_label_layout(s_brightness_value, &niulai_font_16, UI_COLOR_ACCENT,
                     158, 134, 52, 24, LV_TEXT_ALIGN_RIGHT);

    s_brightness_track = lv_obj_create(s_screen);
    lv_obj_set_pos(s_brightness_track, 34, 167);
    lv_obj_set_size(s_brightness_track, SETTINGS_TRACK_WIDTH, 8);
    lv_obj_remove_flag(s_brightness_track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_brightness_track, 0, 0);
    lv_obj_set_style_border_width(s_brightness_track, 0, 0);
    lv_obj_set_style_radius(s_brightness_track, 4, 0);
    lv_obj_set_style_bg_color(s_brightness_track, lv_color_hex(UI_COLOR_TRACK), 0);
    lv_obj_set_style_bg_opa(s_brightness_track, LV_OPA_COVER, 0);

    s_brightness_fill = lv_obj_create(s_brightness_track);
    lv_obj_set_pos(s_brightness_fill, 0, 0);
    lv_obj_set_size(s_brightness_fill, 0, 8);
    lv_obj_remove_flag(s_brightness_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_brightness_fill, 0, 0);
    lv_obj_set_style_border_width(s_brightness_fill, 0, 0);
    lv_obj_set_style_radius(s_brightness_fill, 4, 0);
    lv_obj_set_style_bg_color(s_brightness_fill, lv_color_hex(UI_COLOR_ACCENT), 0);
    lv_obj_set_style_bg_opa(s_brightness_fill, LV_OPA_COVER, 0);

    s_settings_divider = lv_obj_create(s_screen);
    lv_obj_set_pos(s_settings_divider, 28, 194);
    lv_obj_set_size(s_settings_divider, 184, 1);
    lv_obj_set_style_pad_all(s_settings_divider, 0, 0);
    lv_obj_set_style_border_width(s_settings_divider, 0, 0);
    lv_obj_set_style_bg_color(s_settings_divider, lv_color_hex(UI_COLOR_EDGE), 0);
    lv_obj_set_style_bg_opa(s_settings_divider, LV_OPA_40, 0);

    s_voice_label = make_label(s_screen, &niulai_font_12, UI_COLOR_MUTED);
    set_label_layout(s_voice_label, &niulai_font_12, UI_COLOR_MUTED,
                     28, 198, 84, 18, LV_TEXT_ALIGN_LEFT);

    s_reset_panel = lv_obj_create(s_screen);
    lv_obj_set_pos(s_reset_panel, 24, 222);
    lv_obj_set_size(s_reset_panel, 192, 36);
    lv_obj_remove_flag(s_reset_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_reset_panel, 0, 0);
    style_surface(s_reset_panel, UI_COLOR_SURFACE_ALT, UI_COLOR_ACCENT,
                  LV_OPA_COVER, 9, 1);
    s_reset_label = make_label(s_reset_panel, &niulai_font_12, UI_COLOR_TEXT);
    lv_obj_set_size(s_reset_label, 176, 18);
    lv_obj_set_style_text_align(s_reset_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_reset_label);

    show_current_page();
    lv_timer_create(animation_tick, ANIMATION_PERIOD_MS, NULL);
    lv_screen_load(s_screen);
}

esp_err_t niulai_app_start(void)
{
    niulai_model_init(&s_model);
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        niulai_preferences_load(&s_model.volume, &s_model.brightness));
    s_volume = s_model.volume;
    s_brightness = s_model.brightness;
    s_preferences_queue = xQueueCreate(1, sizeof(preferences_update_t));
    if (!s_preferences_queue ||
        xTaskCreate(preferences_task, "niulai_preferences", 2048, NULL, 1,
                    NULL) != pdPASS) {
        ESP_LOGW(TAG, "Settings persistence task unavailable");
        if (s_preferences_queue) {
            vQueueDelete(s_preferences_queue);
            s_preferences_queue = NULL;
        }
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(niulai_voice_store_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_i2c_init());

    if (bsp_display_init() != ESP_OK || bsp_lvgl_init() == NULL) {
        ESP_LOGE(TAG, "Display initialization failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return ESP_FAIL;
    }
    bsp_display_backlight(s_brightness);

    s_audio_ok = bsp_audio_init() == ESP_OK;
    if (s_audio_ok) {
        s_audio_queue = xQueueCreate(1, sizeof(audio_command_t));
        if (!s_audio_queue ||
            xTaskCreate(audio_task, "niulai_audio", 6144, NULL, 4, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Audio task creation failed");
            if (s_audio_queue) {
                vQueueDelete(s_audio_queue);
                s_audio_queue = NULL;
            }
            s_audio_ok = false;
        }
    } else {
        ESP_LOGW(TAG, "Audio unavailable; animation remains usable");
    }

    s_battery_ok = bsp_battery_init() == ESP_OK;
    if (s_battery_ok) s_battery_soc = bsp_battery_soc();

    if (!bsp_lvgl_lock(1000)) return ESP_ERR_TIMEOUT;
    build_ui();
    bsp_lvgl_unlock();

    if (xTaskCreate(battery_task, "niulai_battery", 3072, NULL, 2, NULL) != pdPASS) {
        ESP_LOGW(TAG, "Battery refresh task creation failed");
    }

    esp_err_t button_err = bsp_button_init(on_button, NULL);
    if (button_err != ESP_OK) {
        ESP_LOGE(TAG, "Button initialization failed: %s", esp_err_to_name(button_err));
        return button_err;
    }

    ESP_LOGI(TAG, "Ready: UP=Niu Lai, DOWN=Mama, hold UP/DOWN=record, "
                  "settings=%u%%/%u%%, "
                  "audio=%d, battery=%d, store=%d",
             s_model.volume, s_model.brightness,
             s_audio_ok, s_battery_ok, niulai_voice_store_available());
    return ESP_OK;
}
