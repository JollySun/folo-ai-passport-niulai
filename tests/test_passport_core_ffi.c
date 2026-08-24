// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "passport_core.h"

#include <assert.h>
#include <stdint.h>

int main(void)
{
    const int16_t source[] = { -30000, -1000, 0, 1000, 30000 };
    int16_t output[5];

    passport_pcm16_scale(output, source, 5, 100);
    for (int i = 0; i < 5; ++i) assert(output[i] == source[i]);

    passport_pcm16_scale(output, source, 5, 255);
    for (int i = 0; i < 5; ++i) assert(output[i] == source[i]);

    passport_pcm16_scale(output, source, 5, 50);
    assert(output[0] == -15000);
    assert(output[1] == -500);
    assert(output[2] == 0);
    assert(output[3] == 500);
    assert(output[4] == 15000);

    passport_pcm16_scale(output, source, 5, 0);
    for (int i = 0; i < 5; ++i) assert(output[i] == 0);

    assert(passport_battery_percent_from_voltage(3200) == 0);
    assert(passport_battery_percent_from_voltage(3300) == 0);
    assert(passport_battery_percent_from_voltage(3500) == 5);
    assert(passport_battery_percent_from_voltage(3750) == 37);
    assert(passport_battery_percent_from_voltage(4000) == 80);
    assert(passport_battery_percent_from_voltage(4154) == 96);
    assert(passport_battery_percent_from_voltage(4300) == 100);
    return 0;
}
