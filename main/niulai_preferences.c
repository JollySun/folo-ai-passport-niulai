// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_preferences.h"

#include "niulai_model.h"
#include "nvs.h"
#include "nvs_flash.h"

#define PREFERENCES_NAMESPACE "niulai"
#define VOLUME_KEY "volume"
#define BRIGHTNESS_KEY "brightness"

static bool valid_volume(uint8_t volume)
{
    return volume <= 100;
}

static bool valid_brightness(uint8_t brightness)
{
    return brightness >= NIULAI_MIN_BRIGHTNESS && brightness <= 100;
}

esp_err_t niulai_preferences_load(uint8_t *volume, uint8_t *brightness)
{
    if (!volume || !brightness) return ESP_ERR_INVALID_ARG;

    esp_err_t error = nvs_flash_init();
    if (error != ESP_OK) return error;

    nvs_handle_t handle;
    error = nvs_open(PREFERENCES_NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (error != ESP_OK) return error;

    uint8_t saved_volume;
    if (nvs_get_u8(handle, VOLUME_KEY, &saved_volume) == ESP_OK &&
        valid_volume(saved_volume)) {
        *volume = saved_volume;
    }

    uint8_t saved_brightness;
    if (nvs_get_u8(handle, BRIGHTNESS_KEY, &saved_brightness) == ESP_OK &&
        valid_brightness(saved_brightness)) {
        *brightness = saved_brightness;
    }

    nvs_close(handle);
    return ESP_OK;
}

esp_err_t niulai_preferences_save(uint8_t volume, uint8_t brightness)
{
    if (!valid_volume(volume) || !valid_brightness(brightness)) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t error = nvs_open(PREFERENCES_NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK) return error;

    error = nvs_set_u8(handle, VOLUME_KEY, volume);
    if (error == ESP_OK) {
        error = nvs_set_u8(handle, BRIGHTNESS_KEY, brightness);
    }
    if (error == ESP_OK) error = nvs_commit(handle);

    nvs_close(handle);
    return error;
}
