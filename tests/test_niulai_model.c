// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_model.h"
#include <assert.h>

int main(void)
{
    niulai_model_t model;
    niulai_model_init(&model);
    assert(model.page == NIULAI_PAGE_HOME);
    assert(model.volume == NIULAI_DEFAULT_VOLUME);

    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_CLICK) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_SETTINGS);
    assert(niulai_model_apply(&model, NIULAI_INPUT_UP_CLICK) == NIULAI_ACTION_VOLUME_CHANGED);
    assert(model.volume == 90);
    assert(niulai_model_apply(&model, NIULAI_INPUT_DOWN_CLICK) == NIULAI_ACTION_VOLUME_CHANGED);
    assert(model.volume == NIULAI_DEFAULT_VOLUME);
    for (int i = 0; i < 10; ++i) niulai_model_apply(&model, NIULAI_INPUT_UP_CLICK);
    assert(model.volume == 100);
    assert(niulai_model_apply(&model, NIULAI_INPUT_UP_CLICK) == NIULAI_ACTION_NONE);
    for (int i = 0; i < 20; ++i) niulai_model_apply(&model, NIULAI_INPUT_DOWN_CLICK);
    assert(model.volume == 0);
    assert(niulai_model_apply(&model, NIULAI_INPUT_DOWN_CLICK) == NIULAI_ACTION_NONE);
    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_CLICK) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_HOME);

    assert(niulai_model_apply(&model, NIULAI_INPUT_UP_CLICK) == NIULAI_ACTION_PLAY_MAMA);
    assert(model.page == NIULAI_PAGE_CALF);
    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_CLICK) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_SETTINGS);
    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_CLICK) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_CALF);
    assert(niulai_model_apply(&model, NIULAI_INPUT_UP_CLICK) == NIULAI_ACTION_PLAY_MAMA);
    assert(model.page == NIULAI_PAGE_CALF);

    assert(niulai_model_apply(&model, NIULAI_INPUT_DOWN_CLICK) == NIULAI_ACTION_PLAY_NIULAI);
    assert(model.page == NIULAI_PAGE_MOTHER);
    assert(niulai_model_apply(&model, NIULAI_INPUT_DOWN_CLICK) == NIULAI_ACTION_PLAY_NIULAI);
    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_CLICK) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_SETTINGS);
    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_CLICK) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_MOTHER);

    assert(niulai_model_apply(&model, NIULAI_INPUT_OK_LONG) == NIULAI_ACTION_STOP_AUDIO);
    assert(model.page == NIULAI_PAGE_HOME);
    assert(model.return_page == NIULAI_PAGE_HOME);
    assert(niulai_model_apply(&model, NIULAI_INPUT_NONE) == NIULAI_ACTION_NONE);
    assert(model.page == NIULAI_PAGE_HOME);
    return 0;
}
