// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_app.h"

#include "niulai_audio_math.h"
#include "niulai_fonts.h"
#include "niulai_model.h"
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
#define AUDIO_CHUNK_SAMPLES 512
#define ANIMATION_PERIOD_MS 120
#define MAMA_ANIMATION_MS 2550
#define NIULAI_ANIMATION_MS 1350
#define BATTERY_REFRESH_MS 1000

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
static QueueHandle_t s_audio_queue;
static bool s_audio_ok;
static bool s_battery_ok;
static int s_battery_soc = -1;
static volatile uint8_t s_volume = NIULAI_DEFAULT_VOLUME;
static volatile record_state_t s_record_state = RECORD_STATE_IDLE;
static volatile niulai_voice_slot_t s_record_slot = NIULAI_VOICE_CALF;
static bool s_record_button_down;
static bsp_btn_t s_record_button;
static bool s_animation_active;
static bool s_animation_frame;
static uint32_t s_animation_left_ms;

static lv_obj_t *make_centered_label(lv_obj_t *parent, const lv_font_t *font,
                                     uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, 224);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static const lv_image_dsc_t *animation_image(bool second_frame)
{
    if (s_model.page == NIULAI_PAGE_CALF) {
        return second_frame ? &CALF_2_IMAGE : &CALF_1_IMAGE;
    }
    return second_frame ? &MOTHER_2_IMAGE : &MOTHER_1_IMAGE;
}

static void set_label_font(lv_obj_t *label, const lv_font_t *font)
{
    lv_obj_set_style_text_font(label, font, 0);
}

static void set_battery_hidden(bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(s_battery_body, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_battery_cap, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(s_battery_body, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_battery_cap, LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_battery_icon(void)
{
    if (!s_battery_body) return;

    int percent = s_battery_soc;
    if (percent < 0) {
        lv_obj_set_width(s_battery_fill, 0);
        lv_label_set_text(s_battery_value, "");
        lv_obj_set_style_border_color(s_battery_body, lv_color_hex(0x71C8C1), 0);
        lv_obj_set_style_bg_opa(s_battery_body, LV_OPA_60, 0);
        return;
    }
    if (percent > 100) percent = 100;

    int fill_width = (percent * 30 + 99) / 100;
    lv_obj_set_width(s_battery_fill, fill_width);
    lv_obj_set_style_bg_color(s_battery_fill,
        lv_color_hex(percent <= 20 ? 0xFF9A64 : 0xFFD36B), 0);
    lv_obj_set_style_border_color(s_battery_body, lv_color_hex(0x71C8C1), 0);
    lv_obj_set_style_bg_opa(s_battery_body, LV_OPA_80, 0);
    lv_label_set_text_fmt(s_battery_value, "%d", percent);
}

static void show_home(void)
{
    s_animation_active = false;
    lv_obj_remove_flag(s_image, LV_OBJ_FLAG_HIDDEN);
    set_battery_hidden(false);
    lv_obj_set_pos(s_image, 0, 0);
    lv_image_set_src(s_image, &HOME_IMAGE);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0xF2F0E8), 0);

    lv_obj_set_pos(s_panel, 5, 205);
    lv_obj_set_size(s_panel, 230, 110);
    lv_obj_set_style_bg_color(s_panel, lv_color_hex(0x071A1E), 0);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_80, 0);
    lv_obj_set_style_border_color(s_panel, lv_color_hex(0x71C8C1), 0);

    set_label_font(s_title, &niulai_font_20);
    set_label_font(s_phrase, &niulai_font_14);
    set_label_font(s_hint, &lv_font_montserrat_14);
    lv_label_set_text(s_title, "牛来");
    lv_obj_set_pos(s_title, 8, 211);
    lv_label_set_text(s_phrase, s_audio_ok ? "UP: 妈妈~~\nDOWN: 牛来!!"
                                              : "UP / DOWN: ANIMATION\nAUDIO UNAVAILABLE");
    lv_obj_set_pos(s_phrase, 8, 238);
    lv_label_set_text(s_hint, "HOLD UP / DOWN: RECORD\nOK: SETTINGS");
    lv_obj_set_pos(s_hint, 8, 278);
    update_battery_icon();
}

static void show_active_page(void)
{
    bool calf = s_model.page == NIULAI_PAGE_CALF;
    bool record_page = (calf && s_record_slot == NIULAI_VOICE_CALF) ||
                       (!calf && s_record_slot == NIULAI_VOICE_MOTHER);
    lv_obj_remove_flag(s_image, LV_OBJ_FLAG_HIDDEN);
    set_battery_hidden(true);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x0A171A), 0);
    lv_obj_set_pos(s_image, 0, 48);
    lv_image_set_src(s_image, animation_image(false));

    lv_obj_set_pos(s_panel, 5, 225);
    lv_obj_set_size(s_panel, 230, 90);
    lv_obj_set_style_bg_color(s_panel, lv_color_hex(0x112A2E), 0);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_panel, lv_color_hex(calf ? 0xFF9A64 : 0xFFD36B), 0);

    set_label_font(s_title, &niulai_font_20);
    set_label_font(s_phrase, &niulai_font_20);
    set_label_font(s_hint, &lv_font_montserrat_10);
    lv_label_set_text(s_title, calf ? "牛来" : "妈妈");
    lv_obj_set_pos(s_title, 8, 14);
    if (record_page && s_record_state == RECORD_STATE_PREPARING) {
        lv_label_set_text(s_phrase, "PREPARING...");
    } else if (record_page && s_record_state == RECORD_STATE_ACTIVE) {
        lv_label_set_text(s_phrase, "RECORDING...");
    } else if (record_page && s_record_state == RECORD_STATE_SAVING) {
        lv_label_set_text(s_phrase, "SAVING...");
    } else if (record_page && s_record_state == RECORD_STATE_SAVED) {
        lv_label_set_text(s_phrase, "VOICE SAVED");
    } else if (record_page && s_record_state == RECORD_STATE_FAILED) {
        lv_label_set_text(s_phrase, "RECORD FAILED");
    } else {
        lv_label_set_text(s_phrase, calf ? "妈妈~~" : "牛来!!");
    }
    lv_obj_align_to(s_phrase, s_panel, LV_ALIGN_CENTER, 0, -2);
    lv_label_set_text(s_hint,
        record_page && (s_record_state == RECORD_STATE_PREPARING ||
                        s_record_state == RECORD_STATE_ACTIVE)
            ? "RELEASE TO SAVE" : "HOLD OK: HOME");
    lv_obj_align_to(s_hint, s_panel, LV_ALIGN_BOTTOM_MID, 0, -4);
}

static void show_settings(void)
{
    s_animation_active = false;
    lv_obj_add_flag(s_image, LV_OBJ_FLAG_HIDDEN);
    set_battery_hidden(true);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x071A1E), 0);

    lv_obj_set_pos(s_panel, 15, 66);
    lv_obj_set_size(s_panel, 210, 188);
    lv_obj_set_style_bg_color(s_panel, lv_color_hex(0x112A2E), 0);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_panel, lv_color_hex(0x71C8C1), 0);

    set_label_font(s_title, &lv_font_montserrat_20);
    set_label_font(s_phrase, &lv_font_montserrat_20);
    set_label_font(s_hint, &lv_font_montserrat_14);
    lv_label_set_text(s_title, "SETTINGS");
    lv_obj_set_pos(s_title, 8, 82);
    int custom_count = (niulai_voice_store_has(NIULAI_VOICE_CALF) ? 1 : 0) +
                       (niulai_voice_store_has(NIULAI_VOICE_MOTHER) ? 1 : 0);
    if (s_record_state == RECORD_STATE_PREPARING) {
        lv_label_set_text_fmt(s_phrase, "VOLUME %d%%\nRESETTING...", s_model.volume);
    } else if (s_record_state == RECORD_STATE_FAILED) {
        lv_label_set_text_fmt(s_phrase, "VOLUME %d%%\nRESET FAILED", s_model.volume);
    } else if (!niulai_voice_store_available()) {
        lv_label_set_text_fmt(s_phrase, "VOLUME %d%%\nVOICE STORE: N/A", s_model.volume);
    } else if (s_record_state == RECORD_STATE_RESET_DONE) {
        lv_label_set_text_fmt(s_phrase, "VOLUME %d%%\nVOICES: DEFAULT", s_model.volume);
    } else {
        lv_label_set_text_fmt(s_phrase, "VOLUME %d%%\nCUSTOM VOICES: %d", s_model.volume,
                              custom_count);
    }
    lv_obj_set_pos(s_phrase, 8, 128);
    lv_label_set_text(s_hint,
        "UP / DOWN: VOLUME\nDOUBLE OK: RESET VOICES\nOK: BACK");
    lv_obj_set_pos(s_hint, 8, 194);
}

static void show_current_page(void)
{
    if (s_model.page == NIULAI_PAGE_HOME) show_home();
    else if (s_model.page == NIULAI_PAGE_SETTINGS) show_settings();
    else show_active_page();
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
    if (!s_animation_active || s_model.page == NIULAI_PAGE_HOME) return;

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

    if (bsp_audio_set_format(AUDIO_SAMPLE_RATE, 16, 1) != ESP_OK) {
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
        if (xQueueReceive(s_audio_queue, &next, 0) == pdTRUE) return next;
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

    if (event == BSP_BTN_DOUBLE && button == BSP_BTN_OK &&
        s_model.page == NIULAI_PAGE_SETTINGS) {
        s_record_state = (s_audio_queue && niulai_voice_store_available())
                       ? RECORD_STATE_PREPARING : RECORD_STATE_FAILED;
        if (bsp_lvgl_lock(500)) {
            show_settings();
            bsp_lvgl_unlock();
        }
        if (s_record_state == RECORD_STATE_PREPARING) {
            audio_request(AUDIO_CMD_RESET_VOICES);
        }
        return;
    }

    niulai_input_t input = map_input(button, event);
    if (input == NIULAI_INPUT_NONE) return;

    if (s_record_state == RECORD_STATE_SAVED ||
        s_record_state == RECORD_STATE_FAILED ||
        s_record_state == RECORD_STATE_RESET_DONE) {
        s_record_state = RECORD_STATE_IDLE;
    }

    if (!bsp_lvgl_lock(500)) return;
    niulai_action_t action = niulai_model_apply(&s_model, input);

    show_current_page();
    if (action == NIULAI_ACTION_PLAY_MAMA || action == NIULAI_ACTION_PLAY_NIULAI) {
        start_animation(action == NIULAI_ACTION_PLAY_MAMA
                        ? MAMA_ANIMATION_MS : NIULAI_ANIMATION_MS);
    }
    bsp_lvgl_unlock();

    if (action == NIULAI_ACTION_PLAY_MAMA) audio_request(AUDIO_CMD_PLAY_MAMA);
    else if (action == NIULAI_ACTION_PLAY_NIULAI) audio_request(AUDIO_CMD_PLAY_NIULAI);
    else if (action == NIULAI_ACTION_STOP_AUDIO) audio_request(AUDIO_CMD_STOP);
    else if (action == NIULAI_ACTION_VOLUME_CHANGED) {
        s_volume = s_model.volume;
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

    s_image = lv_image_create(s_screen);
    s_panel = lv_obj_create(s_screen);
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_panel, 7, 0);
    lv_obj_set_style_border_width(s_panel, 2, 0);
    lv_obj_set_style_pad_all(s_panel, 0, 0);

    s_title = make_centered_label(s_screen, &lv_font_montserrat_20, 0xFFFFFF);
    s_phrase = make_centered_label(s_screen, &lv_font_montserrat_14, 0xFFFFFF);
    s_hint = make_centered_label(s_screen, &lv_font_montserrat_14, 0xB8D8D8);

    // iOS-style status battery: percentage sits inside the rounded battery
    // body, with a fill level and a separate terminal instead of "BAT x%".
    s_battery_body = lv_obj_create(s_screen);
    lv_obj_set_pos(s_battery_body, 8, 8);
    lv_obj_set_size(s_battery_body, 36, 16);
    lv_obj_remove_flag(s_battery_body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_battery_body, 0, 0);
    lv_obj_set_style_radius(s_battery_body, 5, 0);
    lv_obj_set_style_border_width(s_battery_body, 2, 0);
    lv_obj_set_style_border_color(s_battery_body, lv_color_hex(0x71C8C1), 0);
    lv_obj_set_style_bg_color(s_battery_body, lv_color_hex(0x071A1E), 0);
    lv_obj_set_style_bg_opa(s_battery_body, LV_OPA_80, 0);

    s_battery_fill = lv_obj_create(s_battery_body);
    lv_obj_set_pos(s_battery_fill, 1, 1);
    lv_obj_set_size(s_battery_fill, 0, 10);
    lv_obj_remove_flag(s_battery_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(s_battery_fill, 0, 0);
    lv_obj_set_style_border_width(s_battery_fill, 0, 0);
    lv_obj_set_style_radius(s_battery_fill, 3, 0);
    lv_obj_set_style_bg_opa(s_battery_fill, LV_OPA_COVER, 0);

    s_battery_value = lv_label_create(s_battery_body);
    lv_obj_set_width(s_battery_value, 32);
    lv_obj_set_style_text_align(s_battery_value, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_battery_value, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s_battery_value, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(s_battery_value, lv_color_hex(0x071A1E), 0);
    lv_obj_set_style_bg_opa(s_battery_value, LV_OPA_60, 0);
    lv_obj_set_style_radius(s_battery_value, 2, 0);
    lv_obj_center(s_battery_value);

    s_battery_cap = lv_obj_create(s_screen);
    lv_obj_set_pos(s_battery_cap, 45, 13);
    lv_obj_set_size(s_battery_cap, 3, 6);
    lv_obj_set_style_pad_all(s_battery_cap, 0, 0);
    lv_obj_set_style_border_width(s_battery_cap, 0, 0);
    lv_obj_set_style_radius(s_battery_cap, 2, 0);
    lv_obj_set_style_bg_color(s_battery_cap, lv_color_hex(0x71C8C1), 0);
    lv_obj_set_style_bg_opa(s_battery_cap, LV_OPA_COVER, 0);

    show_home();
    lv_timer_create(animation_tick, ANIMATION_PERIOD_MS, NULL);
    lv_screen_load(s_screen);
}

esp_err_t niulai_app_start(void)
{
    niulai_model_init(&s_model);
    s_volume = s_model.volume;
    ESP_ERROR_CHECK_WITHOUT_ABORT(niulai_voice_store_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_i2c_init());

    if (bsp_display_init() != ESP_OK || bsp_lvgl_init() == NULL) {
        ESP_LOGE(TAG, "Display initialization failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return ESP_FAIL;
    }
    bsp_display_backlight(100);

    s_audio_ok = bsp_audio_init() == ESP_OK;
    if (s_audio_ok) {
        s_audio_queue = xQueueCreate(1, sizeof(audio_command_t));
        if (!s_audio_queue || xTaskCreate(audio_task, "niulai_audio", 6144, NULL, 4, NULL) != pdPASS) {
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
                  "double OK in settings=reset voices, audio=%d, battery=%d, store=%d",
             s_audio_ok, s_battery_ok, niulai_voice_store_available());
    return ESP_OK;
}
