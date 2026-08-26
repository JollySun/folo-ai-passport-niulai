#pragma once

#include "esp_err.h"

esp_err_t bsp_battery_init(void);
int bsp_battery_soc(void);
