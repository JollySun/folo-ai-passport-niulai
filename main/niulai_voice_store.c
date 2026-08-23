// SPDX-License-Identifier: MIT
// Copyright (c) 2026 FoloToy

#include "niulai_voice_store.h"

#include "esp_log.h"
#include "esp_partition.h"
#include <string.h>

#define VOICE_PARTITION_SUBTYPE ((esp_partition_subtype_t)0x40)
#define VOICE_MAGIC 0x4E4C5652U
#define VOICE_VERSION 1U
#define VOICE_SLOT_BYTES 0x100000U
#define VOICE_BANK_BYTES 0x80000U
#define VOICE_BANK_COUNT 2
#define VOICE_HEADER_BYTES 32U
#define VOICE_MIN_BYTES 3200U

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t sample_rate;
    uint32_t sequence;
    uint32_t reserved[3];
} voice_header_t;

_Static_assert(sizeof(voice_header_t) == VOICE_HEADER_BYTES,
               "voice header must remain 32 bytes");

static const char *TAG = "voice_store";
static const esp_partition_t *s_partition;
static voice_header_t s_headers[NIULAI_VOICE_COUNT];
static int s_active_bank[NIULAI_VOICE_COUNT] = { -1, -1 };
static int s_recording_slot = -1;
static int s_recording_bank = -1;
static size_t s_recording_bytes;

static size_t slot_offset(niulai_voice_slot_t slot)
{
    return (size_t)slot * VOICE_SLOT_BYTES;
}

static size_t bank_offset(niulai_voice_slot_t slot, int bank)
{
    return slot_offset(slot) + (size_t)bank * VOICE_BANK_BYTES;
}

static bool header_valid(const voice_header_t *header)
{
    return header->magic == VOICE_MAGIC &&
           header->version == VOICE_VERSION &&
           header->sample_rate == NIULAI_RECORD_SAMPLE_RATE &&
           header->length >= VOICE_MIN_BYTES &&
           header->length <= NIULAI_RECORD_MAX_BYTES &&
           (header->length % 2U) == 0;
}

esp_err_t niulai_voice_store_init(void)
{
    s_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                            VOICE_PARTITION_SUBTYPE,
                                            "recordings");
    if (!s_partition || s_partition->size < VOICE_SLOT_BYTES * NIULAI_VOICE_COUNT) {
        ESP_LOGW(TAG, "recordings partition unavailable; flash merged firmware first");
        s_partition = NULL;
        return ESP_ERR_NOT_FOUND;
    }

    memset(s_headers, 0, sizeof(s_headers));
    for (int i = 0; i < NIULAI_VOICE_COUNT; ++i) s_active_bank[i] = -1;
    for (int i = 0; i < NIULAI_VOICE_COUNT; ++i) {
        for (int bank = 0; bank < VOICE_BANK_COUNT; ++bank) {
            voice_header_t header;
            if (esp_partition_read(s_partition,
                                   bank_offset((niulai_voice_slot_t)i, bank),
                                   &header, sizeof(header)) == ESP_OK &&
                header_valid(&header) &&
                (s_active_bank[i] < 0 ||
                 header.sequence > s_headers[i].sequence)) {
                s_headers[i] = header;
                s_active_bank[i] = bank;
            }
        }
    }
    ESP_LOGI(TAG, "recordings ready: calf=%lu mother=%lu",
             (unsigned long)s_headers[NIULAI_VOICE_CALF].length,
             (unsigned long)s_headers[NIULAI_VOICE_MOTHER].length);
    return ESP_OK;
}

bool niulai_voice_store_available(void)
{
    return s_partition != NULL;
}

bool niulai_voice_store_has(niulai_voice_slot_t slot)
{
    return slot >= 0 && slot < NIULAI_VOICE_COUNT &&
           header_valid(&s_headers[slot]);
}

size_t niulai_voice_store_length(niulai_voice_slot_t slot)
{
    return niulai_voice_store_has(slot) ? s_headers[slot].length : 0;
}

esp_err_t niulai_voice_store_read(niulai_voice_slot_t slot, size_t offset,
                                  void *buffer, size_t bytes)
{
    size_t length = niulai_voice_store_length(slot);
    if (!s_partition || !niulai_voice_store_has(slot) || !buffer || bytes == 0 ||
        offset > length || bytes > length - offset) {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_partition_read(s_partition,
                              bank_offset(slot, s_active_bank[slot]) +
                                  VOICE_HEADER_BYTES + offset,
                              buffer, bytes);
}

esp_err_t niulai_voice_store_begin(niulai_voice_slot_t slot)
{
    if (!s_partition || slot < 0 || slot >= NIULAI_VOICE_COUNT) {
        return ESP_ERR_INVALID_STATE;
    }
    int bank = s_active_bank[slot] == 0 ? 1 : 0;
    esp_err_t error = esp_partition_erase_range(s_partition,
                                                 bank_offset(slot, bank),
                                                 VOICE_BANK_BYTES);
    if (error != ESP_OK) return error;

    s_recording_slot = slot;
    s_recording_bank = bank;
    s_recording_bytes = 0;
    return ESP_OK;
}

esp_err_t niulai_voice_store_append(const void *pcm, size_t bytes)
{
    if (!s_partition || s_recording_slot < 0 || !pcm || bytes == 0 ||
        bytes > NIULAI_RECORD_MAX_BYTES - s_recording_bytes) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t error = esp_partition_write(
        s_partition,
        bank_offset((niulai_voice_slot_t)s_recording_slot, s_recording_bank) +
            VOICE_HEADER_BYTES + s_recording_bytes,
        pcm, bytes);
    if (error == ESP_OK) s_recording_bytes += bytes;
    return error;
}

esp_err_t niulai_voice_store_finish(void)
{
    if (!s_partition || s_recording_slot < 0 ||
        s_recording_bytes < VOICE_MIN_BYTES) {
        niulai_voice_store_cancel();
        return ESP_ERR_INVALID_SIZE;
    }

    niulai_voice_slot_t slot = (niulai_voice_slot_t)s_recording_slot;
    voice_header_t header = {
        .magic = VOICE_MAGIC,
        .version = VOICE_VERSION,
        .length = (uint32_t)s_recording_bytes,
        .sample_rate = NIULAI_RECORD_SAMPLE_RATE,
        .sequence = s_active_bank[slot] < 0 ? 1U : s_headers[slot].sequence + 1U,
    };
    esp_err_t error = esp_partition_write(s_partition,
                                           bank_offset(slot, s_recording_bank),
                                           &header, sizeof(header));
    if (error == ESP_OK) {
        s_headers[slot] = header;
        s_active_bank[slot] = s_recording_bank;
        ESP_LOGI(TAG, "saved slot=%d bank=%d bytes=%lu", slot,
                 s_recording_bank, (unsigned long)s_recording_bytes);
    }
    s_recording_slot = -1;
    s_recording_bank = -1;
    s_recording_bytes = 0;
    return error;
}

void niulai_voice_store_cancel(void)
{
    s_recording_slot = -1;
    s_recording_bank = -1;
    s_recording_bytes = 0;
}

esp_err_t niulai_voice_store_reset(void)
{
    if (!s_partition) return ESP_ERR_INVALID_STATE;
    niulai_voice_store_cancel();
    for (int i = 0; i < NIULAI_VOICE_COUNT; ++i) {
        for (int bank = 0; bank < VOICE_BANK_COUNT; ++bank) {
            esp_err_t error = esp_partition_erase_range(
                s_partition, bank_offset((niulai_voice_slot_t)i, bank),
                s_partition->erase_size);
            if (error != ESP_OK) return error;
        }
    }
    memset(s_headers, 0, sizeof(s_headers));
    for (int i = 0; i < NIULAI_VOICE_COUNT; ++i) s_active_bank[i] = -1;
    ESP_LOGI(TAG, "custom voices reset to defaults");
    return ESP_OK;
}
