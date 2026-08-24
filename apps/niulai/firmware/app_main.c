// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_app.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Niu Lai interactive app");
    if (niulai_app_start() != ESP_OK) {
        ESP_LOGE(TAG, "Niu Lai app failed to start");
    }
}
