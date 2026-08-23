// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

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
} niulai_action_t;

typedef struct {
    niulai_page_t page;
    niulai_page_t return_page;
    unsigned char volume;
} niulai_model_t;

#define NIULAI_DEFAULT_VOLUME 85
#define NIULAI_VOLUME_STEP 5

void niulai_model_init(niulai_model_t *model);
niulai_action_t niulai_model_apply(niulai_model_t *model, niulai_input_t input);
