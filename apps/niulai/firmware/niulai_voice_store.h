// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NIULAI_VOICE_CALF = 0,
    NIULAI_VOICE_MOTHER,
    NIULAI_VOICE_COUNT,
} niulai_voice_slot_t;

#define NIULAI_RECORD_SAMPLE_RATE 16000
#define NIULAI_RECORD_MAX_SECONDS 10
#define NIULAI_RECORD_MAX_BYTES \
    (NIULAI_RECORD_SAMPLE_RATE * 2 * NIULAI_RECORD_MAX_SECONDS)

esp_err_t niulai_voice_store_init(void);
bool niulai_voice_store_available(void);
bool niulai_voice_store_has(niulai_voice_slot_t slot);
size_t niulai_voice_store_length(niulai_voice_slot_t slot);
esp_err_t niulai_voice_store_read(niulai_voice_slot_t slot, size_t offset,
                                  void *buffer, size_t bytes);

esp_err_t niulai_voice_store_begin(niulai_voice_slot_t slot);
esp_err_t niulai_voice_store_append(const void *pcm, size_t bytes);
esp_err_t niulai_voice_store_finish(void);
void niulai_voice_store_cancel(void);
esp_err_t niulai_voice_store_reset(void);
