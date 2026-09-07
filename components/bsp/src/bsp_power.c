// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "bsp_power.h"

#include "bsp_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "bsp_power";

esp_err_t bsp_power_enter_deep_sleep(void)
{
    const uint64_t button_mask = 1ULL << BSP_BTN_GPIO;
    const gpio_config_t config = {
        .pin_bit_mask = button_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t error = gpio_config(&config);
    if (error != ESP_OK) return error;

    // GPIO0 是本板三键公共 ADC 节点，外部 10k 上拉保证松开时为高电平。
    error = esp_deep_sleep_enable_gpio_wakeup(button_mask,
                                               ESP_GPIO_WAKEUP_GPIO_LOW);
    if (error != ESP_OK) return error;

    ESP_LOGI(TAG, "进入深睡，GPIO%d 低电平唤醒", BSP_BTN_GPIO);
    esp_deep_sleep_start();
    return ESP_OK;
}

bool bsp_power_woke_from_deep_sleep(void)
{
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO;
}
