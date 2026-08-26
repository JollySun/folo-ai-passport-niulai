// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#pragma once

#include "esp_err.h"
#include <stdint.h>

// Loads valid saved values into the supplied defaults. Missing or invalid
// values leave the caller-provided values unchanged.
esp_err_t niulai_preferences_load(uint8_t *volume, uint8_t *brightness);

esp_err_t niulai_preferences_save(uint8_t volume, uint8_t brightness);
