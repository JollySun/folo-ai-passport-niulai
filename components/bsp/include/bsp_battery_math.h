// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

// Conservative single-cell Li-ion voltage estimate used only while the fuel
// gauge's SOC register is not ready. Input is millivolts; output is 0..100.
int bsp_battery_percent_from_voltage(int millivolts);
