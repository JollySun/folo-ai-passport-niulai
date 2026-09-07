// SPDX-License-Identifier: MIT

#include "niulai_fonts.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

const lv_font_t lv_font_montserrat_12 = {0};

const void *lv_font_get_bitmap_fmt_txt(lv_font_glyph_dsc_t *glyph,
                                       lv_draw_buf_t *draw_buf)
{
    (void)glyph;
    (void)draw_buf;
    return NULL;
}

static int find_codepoint(const uint16_t *list, uint16_t length,
                          uint16_t target)
{
    int low = 0;
    int high = length - 1;
    while (low <= high) {
        int middle = low + (high - low) / 2;
        if (list[middle] == target) return middle;
        if (list[middle] < target) low = middle + 1;
        else high = middle - 1;
    }
    return -1;
}

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t *font,
                                   lv_font_glyph_dsc_t *glyph,
                                   uint32_t letter, uint32_t next_letter)
{
    (void)next_letter;
    const lv_font_fmt_txt_dsc_t *font_dsc = font->dsc;

    for (uint16_t i = 0; i < font_dsc->cmap_num; ++i) {
        const lv_font_fmt_txt_cmap_t *cmap = &font_dsc->cmaps[i];
        if (letter < cmap->range_start ||
            letter >= cmap->range_start + cmap->range_length) {
            continue;
        }

        uint16_t relative = (uint16_t)(letter - cmap->range_start);
        uint16_t glyph_id;
        if (cmap->type == LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY) {
            glyph_id = cmap->glyph_id_start + relative;
        } else if (cmap->type == LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL) {
            const uint8_t *offsets = cmap->glyph_id_ofs_list;
            glyph_id = cmap->glyph_id_start + offsets[relative];
        } else {
            int index = find_codepoint(cmap->unicode_list,
                                       cmap->list_length, relative);
            if (index < 0) return false;
            if (cmap->type == LV_FONT_FMT_TXT_CMAP_SPARSE_TINY) {
                glyph_id = cmap->glyph_id_start + (uint16_t)index;
            } else {
                const uint16_t *offsets = cmap->glyph_id_ofs_list;
                glyph_id = cmap->glyph_id_start + offsets[index];
            }
        }

        const lv_font_fmt_txt_glyph_dsc_t *source =
            &font_dsc->glyph_dsc[glyph_id];
        glyph->adv_w = (source->adv_w + 8) >> 4;
        glyph->box_w = source->box_w;
        glyph->box_h = source->box_h;
        glyph->ofs_x = source->ofs_x;
        glyph->ofs_y = source->ofs_y;
        glyph->format = font_dsc->bpp;
        glyph->gid.index = glyph_id;
        return true;
    }
    return false;
}

static uint32_t next_codepoint(const char **text)
{
    const uint8_t *input = (const uint8_t *)*text;
    uint32_t codepoint;
    size_t length;

    if (input[0] < 0x80) {
        codepoint = input[0];
        length = 1;
    } else if ((input[0] & 0xE0) == 0xC0) {
        codepoint = ((uint32_t)(input[0] & 0x1F) << 6) |
                    (input[1] & 0x3F);
        length = 2;
    } else if ((input[0] & 0xF0) == 0xE0) {
        codepoint = ((uint32_t)(input[0] & 0x0F) << 12) |
                    ((uint32_t)(input[1] & 0x3F) << 6) |
                    (input[2] & 0x3F);
        length = 3;
    } else {
        codepoint = ((uint32_t)(input[0] & 0x07) << 18) |
                    ((uint32_t)(input[1] & 0x3F) << 12) |
                    ((uint32_t)(input[2] & 0x3F) << 6) |
                    (input[3] & 0x3F);
        length = 4;
    }

    *text += length;
    return codepoint;
}

static void assert_font_supports(const char *text)
{
    while (*text) {
        uint32_t codepoint = next_codepoint(&text);
        if (codepoint < 0x80) continue;

        lv_font_glyph_dsc_t glyph;
        assert(niulai_font_12.get_glyph_dsc(&niulai_font_12, &glyph,
                                            codepoint, 0));
    }
}

int main(void)
{
    assert_font_supports("上下键  ·  确认键进入\n长按确认键返回");
    assert_font_supports("上键增加  ·  下键减少\n长按确认键返回");
    assert_font_supports("确认键恢复  ·  上下键返回\n长按确认键返回");
    assert_font_supports("《牛来》");
    return 0;
}
