#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

esp_err_t bsp_audio_init(void);
esp_err_t bsp_audio_set_format(uint32_t hz, uint8_t bits, uint8_t channels);
esp_err_t bsp_audio_write(const void *pcm, size_t bytes);
esp_err_t bsp_audio_read(void *pcm, size_t bytes);
void bsp_audio_set_volume(uint8_t percent);
esp_err_t bsp_audio_suspend(void);
esp_err_t bsp_audio_resume(void);
