// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_model.h"

void niulai_model_init(niulai_model_t *model)
{
    model->page = NIULAI_PAGE_HOME;
    model->return_page = NIULAI_PAGE_HOME;
    model->volume = NIULAI_DEFAULT_VOLUME;
    model->brightness = NIULAI_DEFAULT_BRIGHTNESS;
    model->setting = NIULAI_SETTING_VOLUME;
    model->setting_active = false;
    model->reset_confirming = false;
}

niulai_action_t niulai_model_apply(niulai_model_t *model, niulai_input_t input)
{
    if (input == NIULAI_INPUT_OK_LONG) {
        if (model->page == NIULAI_PAGE_SETTINGS) {
            model->page = model->return_page;
        } else {
            model->page = NIULAI_PAGE_HOME;
            model->return_page = NIULAI_PAGE_HOME;
        }
        model->setting_active = false;
        model->reset_confirming = false;
        return NIULAI_ACTION_STOP_AUDIO;
    }

    if (input == NIULAI_INPUT_OK_CLICK) {
        if (model->page == NIULAI_PAGE_SETTINGS) {
            if (model->setting == NIULAI_SETTING_RESET_VOICES) {
                if (model->reset_confirming) {
                    model->reset_confirming = false;
                    return NIULAI_ACTION_RESET_VOICES;
                }
                model->reset_confirming = true;
                return NIULAI_ACTION_NONE;
            }
            model->setting_active = !model->setting_active;
        } else {
            model->return_page = model->page;
            model->page = NIULAI_PAGE_SETTINGS;
            model->setting = NIULAI_SETTING_VOLUME;
            model->setting_active = false;
            model->reset_confirming = false;
        }
        return NIULAI_ACTION_NONE;
    }

    if (model->page == NIULAI_PAGE_SETTINGS) {
        if (input != NIULAI_INPUT_UP_CLICK &&
            input != NIULAI_INPUT_DOWN_CLICK) {
            return NIULAI_ACTION_NONE;
        }

        if (model->reset_confirming) {
            model->reset_confirming = false;
            return NIULAI_ACTION_NONE;
        }

        if (!model->setting_active) {
            int direction = input == NIULAI_INPUT_UP_CLICK ? -1 : 1;
            int next = (int)model->setting + direction;
            if (next < 0) next = NIULAI_SETTING_COUNT - 1;
            if (next >= NIULAI_SETTING_COUNT) next = 0;
            model->setting = (niulai_setting_t)next;
            return NIULAI_ACTION_NONE;
        }

        if (model->setting == NIULAI_SETTING_VOLUME) {
            if (input == NIULAI_INPUT_UP_CLICK && model->volume < 100) {
                unsigned int next = model->volume + NIULAI_VOLUME_STEP;
                model->volume = next > 100 ? 100 : (uint8_t)next;
                return NIULAI_ACTION_VOLUME_CHANGED;
            }
            if (input == NIULAI_INPUT_DOWN_CLICK && model->volume > 0) {
                model->volume = model->volume < NIULAI_VOLUME_STEP
                              ? 0 : model->volume - NIULAI_VOLUME_STEP;
                return NIULAI_ACTION_VOLUME_CHANGED;
            }
        } else if (model->setting == NIULAI_SETTING_BRIGHTNESS) {
            if (input == NIULAI_INPUT_UP_CLICK && model->brightness < 100) {
                unsigned int next = model->brightness + NIULAI_BRIGHTNESS_STEP;
                model->brightness = next > 100 ? 100 : (uint8_t)next;
                return NIULAI_ACTION_BRIGHTNESS_CHANGED;
            }
            if (input == NIULAI_INPUT_DOWN_CLICK &&
                model->brightness > NIULAI_MIN_BRIGHTNESS) {
                model->brightness =
                    model->brightness < NIULAI_MIN_BRIGHTNESS +
                                       NIULAI_BRIGHTNESS_STEP
                        ? NIULAI_MIN_BRIGHTNESS
                        : model->brightness - NIULAI_BRIGHTNESS_STEP;
                return NIULAI_ACTION_BRIGHTNESS_CHANGED;
            }
        }
        return NIULAI_ACTION_NONE;
    }

    switch (input) {
    case NIULAI_INPUT_UP_CLICK:
        model->page = NIULAI_PAGE_CALF;
        return NIULAI_ACTION_PLAY_MAMA;
    case NIULAI_INPUT_DOWN_CLICK:
        model->page = NIULAI_PAGE_MOTHER;
        return NIULAI_ACTION_PLAY_NIULAI;
    case NIULAI_INPUT_OK_CLICK:
    case NIULAI_INPUT_OK_LONG:
    case NIULAI_INPUT_NONE:
    default:
        return NIULAI_ACTION_NONE;
    }
}
