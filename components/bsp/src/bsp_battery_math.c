// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "bsp_battery_math.h"

#include <stddef.h>

int bsp_battery_percent_from_voltage(int mv)
{
    static const struct { int mv; int percent; } curve[] = {
        { 3300, 0 }, { 3500, 5 }, { 3600, 10 }, { 3700, 25 }, { 3800, 50 },
        { 3900, 65 }, { 4000, 80 }, { 4100, 92 }, { 4200, 100 },
    };
    if (mv <= curve[0].mv) return 0;
    for (size_t i = 1; i < sizeof(curve) / sizeof(curve[0]); ++i) {
        if (mv <= curve[i].mv) {
            int span_mv = curve[i].mv - curve[i - 1].mv;
            int span_percent = curve[i].percent - curve[i - 1].percent;
            return curve[i - 1].percent +
                   (mv - curve[i - 1].mv) * span_percent / span_mv;
        }
    }
    return 100;
}
