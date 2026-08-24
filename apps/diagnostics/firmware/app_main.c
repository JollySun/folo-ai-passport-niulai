// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "diagnostics.h"

#include "bsp_battery.h"
#include "bsp_i2c.h"
#include "esp_log.h"

static const char *TAG = "diagnostics";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting board diagnostics");

    esp_err_t i2c_error = bsp_i2c_init();
    if (i2c_error != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s",
                 esp_err_to_name(i2c_error));
        return;
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_i2c_scan());

    esp_err_t battery_error = bsp_battery_init();
    if (battery_error != ESP_OK) {
        ESP_LOGW(TAG, "Battery gauge unavailable: %s; Rust core 3750mV=%d%%",
                 esp_err_to_name(battery_error),
                 diagnostics_voltage_percent(3750));
        return;
    }

    int millivolts = bsp_battery_mv();
    int percent = bsp_battery_soc();
    ESP_LOGI(TAG, "Battery: %dmV, SOC=%d%%, voltage estimate=%d%%",
             millivolts, percent,
             diagnostics_voltage_percent(millivolts));
}
