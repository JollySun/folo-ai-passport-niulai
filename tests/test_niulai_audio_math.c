// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_audio_math.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    const int16_t source[] = { -30000, -1000, 0, 1000, 30000 };
    int16_t output[5];

    niulai_scale_pcm16(output, source, 5, 100);
    for (int i = 0; i < 5; ++i) assert(output[i] == source[i]);

    niulai_scale_pcm16(output, source, 5, 50);
    assert(output[0] == -15000);
    assert(output[1] == -500);
    assert(output[2] == 0);
    assert(output[3] == 500);
    assert(output[4] == 15000);

    niulai_scale_pcm16(output, source, 5, 0);
    for (int i = 0; i < 5; ++i) assert(output[i] == 0);
    return 0;
}
