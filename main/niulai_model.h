// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NIULAI_PAGE_HOME,
    NIULAI_PAGE_CALF,
    NIULAI_PAGE_MOTHER,
    NIULAI_PAGE_SETTINGS,
} niulai_page_t;

typedef enum {
    NIULAI_INPUT_NONE,
    NIULAI_INPUT_UP_CLICK,
    NIULAI_INPUT_DOWN_CLICK,
    NIULAI_INPUT_OK_CLICK,
    NIULAI_INPUT_OK_LONG,
} niulai_input_t;

typedef enum {
    NIULAI_ACTION_NONE,
    NIULAI_ACTION_PLAY_MAMA,
    NIULAI_ACTION_PLAY_NIULAI,
    NIULAI_ACTION_STOP_AUDIO,
    NIULAI_ACTION_VOLUME_CHANGED,
    NIULAI_ACTION_BRIGHTNESS_CHANGED,
    NIULAI_ACTION_RESET_VOICES,
} niulai_action_t;

typedef enum {
    NIULAI_SETTING_VOLUME,
    NIULAI_SETTING_BRIGHTNESS,
    NIULAI_SETTING_RESET_VOICES,
    NIULAI_SETTING_COUNT,
} niulai_setting_t;

typedef struct {
    niulai_page_t page;
    niulai_page_t return_page;
    uint8_t volume;
    uint8_t brightness;
    niulai_setting_t setting;
    bool setting_active;
    bool reset_confirming;
} niulai_model_t;

#define NIULAI_DEFAULT_VOLUME 45
#define NIULAI_VOLUME_STEP 5
#define NIULAI_DEFAULT_BRIGHTNESS 70
#define NIULAI_MIN_BRIGHTNESS 20
#define NIULAI_BRIGHTNESS_STEP 10

void niulai_model_init(niulai_model_t *model);
niulai_action_t niulai_model_apply(niulai_model_t *model, niulai_input_t input);
