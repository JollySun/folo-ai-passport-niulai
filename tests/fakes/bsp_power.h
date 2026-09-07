#pragma once

#include "esp_err.h"
#include <stdbool.h>

esp_err_t bsp_power_enter_deep_sleep(void);
bool bsp_power_woke_from_deep_sleep(void);
