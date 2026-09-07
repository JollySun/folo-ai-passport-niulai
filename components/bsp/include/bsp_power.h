// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include "esp_err.h"
#include <stdbool.h>

// 配置 GPIO0 低电平作为深睡唤醒源，并进入 ESP32-C3 深睡。
// 正常情况下该函数不会返回；若硬件/配置失败则返回错误。
esp_err_t bsp_power_enter_deep_sleep(void);

// 判断本次启动是否由深睡 GPIO 唤醒。
bool bsp_power_woke_from_deep_sleep(void);
