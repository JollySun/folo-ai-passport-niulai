// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_model.h"

void niulai_model_init(niulai_model_t *model)
{
    model->page = NIULAI_PAGE_HOME;
    model->return_page = NIULAI_PAGE_HOME;
    model->volume = NIULAI_DEFAULT_VOLUME;
}

niulai_action_t niulai_model_apply(niulai_model_t *model, niulai_input_t input)
{
    if (input == NIULAI_INPUT_OK_LONG) {
        model->page = NIULAI_PAGE_HOME;
        model->return_page = NIULAI_PAGE_HOME;
        return NIULAI_ACTION_STOP_AUDIO;
    }

    if (input == NIULAI_INPUT_OK_CLICK) {
        if (model->page == NIULAI_PAGE_SETTINGS) {
            model->page = model->return_page;
        } else {
            model->return_page = model->page;
            model->page = NIULAI_PAGE_SETTINGS;
        }
        return NIULAI_ACTION_NONE;
    }

    if (model->page == NIULAI_PAGE_SETTINGS) {
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
