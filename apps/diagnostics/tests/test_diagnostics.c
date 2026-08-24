// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "diagnostics.h"

#include <assert.h>

int main(void)
{
    assert(diagnostics_voltage_percent(3200) == 0);
    assert(diagnostics_voltage_percent(3750) == 37);
    assert(diagnostics_voltage_percent(4300) == 100);
    return 0;
}
