// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include <stddef.h>
#include <stdint.h>

void passport_pcm16_scale(int16_t *output, const int16_t *input,
                          size_t sample_count, uint8_t volume);

// Conservative single-cell Li-ion estimate. Input is millivolts; output is
// clamped to 0..100.
int passport_battery_percent_from_voltage(int millivolts);
