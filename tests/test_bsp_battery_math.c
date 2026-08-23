// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "bsp_battery_math.h"

#include <assert.h>

int main(void)
{
    assert(bsp_battery_percent_from_voltage(3200) == 0);
    assert(bsp_battery_percent_from_voltage(3300) == 0);
    assert(bsp_battery_percent_from_voltage(3500) == 5);
    assert(bsp_battery_percent_from_voltage(3750) == 37);
    assert(bsp_battery_percent_from_voltage(4000) == 80);
    assert(bsp_battery_percent_from_voltage(4154) == 96);
    assert(bsp_battery_percent_from_voltage(4300) == 100);
    return 0;
}
