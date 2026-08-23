// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_audio_math.h"

void niulai_scale_pcm16(int16_t *output, const int16_t *input,
                        size_t sample_count, uint8_t volume)
{
    uint8_t gain = volume > 100 ? 100 : volume;
    for (size_t i = 0; i < sample_count; ++i) {
        output[i] = (int16_t)(((int32_t)input[i] * gain) / 100);
    }
}
