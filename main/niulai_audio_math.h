// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include <stddef.h>
#include <stdint.h>

void niulai_scale_pcm16(int16_t *output, const int16_t *input,
                        size_t sample_count, uint8_t volume);
