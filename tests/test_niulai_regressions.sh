#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 FoloToy

set -u

failed=0

check_source() {
    pattern="$1"
    file="$2"
    message="$3"
    if ! grep -q "$pattern" "$file"; then
        echo "FAIL: $message"
        failed=1
    fi
}

check_source 'cw_write(CW_REG_CONFIG, 0x30)' components/bsp/src/bsp_battery.c \
    'CW2017 wake sequence is missing the required 0x30 step'
check_source 'niulai_scale_pcm16' main/niulai_app.c \
    'playback does not apply software PCM volume scaling'
check_source 'mother-2-open.rgb565' main/CMakeLists.txt \
    'the visible open-mouth mother frame is not embedded'
check_source 'lv_font_montserrat_10' main/niulai_app.c \
    'active-page HOLD OK hint is not using the smaller font'
check_source 'lv_obj_align_to(s_phrase, s_panel, LV_ALIGN_CENTER' main/niulai_app.c \
    'active-page phrase is not centered against its panel'
check_source 's_battery_fill' main/niulai_app.c \
    'home page is not using a fill-based iOS battery icon'
check_source 'lv_obj_set_width(s_battery_fill' main/niulai_app.c \
    'battery icon fill does not track the reported percentage'
if grep -q 'BAT %' main/niulai_app.c; then
    echo 'FAIL: legacy BAT x% text is still present'
    failed=1
fi
check_source 'bsp_battery_percent_from_voltage' components/bsp/src/bsp_battery.c \
    'battery SOC has no voltage fallback for an unready gauge'
check_source 'version after wake' components/bsp/src/bsp_battery.c \
    'CW2017 identity is still checked before the device is awake'
check_source 'recordings, data, 0x40' partitions.csv \
    'persistent recordings partition is missing'
check_source 'BUTTON_PRESS_UP' components/bsp/src/bsp_button.c \
    'button release events are unavailable for ending a recording'
check_source 'AUDIO_CMD_RECORD_CALF' main/niulai_app.c \
    'calf-page custom voice recording is not wired'
check_source 'AUDIO_CMD_RECORD_MOTHER' main/niulai_app.c \
    'mother-page custom voice recording is not wired'
check_source 'AUDIO_CMD_RESET_VOICES' main/niulai_app.c \
    'settings cannot reset custom voices'
check_source 'niulai_voice_store_read' main/niulai_app.c \
    'playback does not read saved custom voices'
check_source 'lv_obj_set_pos(s_battery_body, 8, 8)' main/niulai_app.c \
    'battery indicator is not positioned at the top left'

if ! cc -std=c11 -Wall -Wextra -Werror -Imain \
    tests/test_niulai_audio_math.c main/niulai_audio_math.c \
    -o /tmp/test_niulai_audio_math 2>/tmp/test_niulai_audio_math.err; then
    echo 'FAIL: PCM volume regression test does not build'
    failed=1
elif ! /tmp/test_niulai_audio_math; then
    echo 'FAIL: PCM volume regression test failed'
    failed=1
fi

if ! cc -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/include \
    tests/test_bsp_battery_math.c components/bsp/src/bsp_battery_math.c \
    -o /tmp/test_bsp_battery_math 2>/tmp/test_bsp_battery_math.err; then
    echo 'FAIL: battery voltage fallback regression test does not build'
    failed=1
elif ! /tmp/test_bsp_battery_math; then
    echo 'FAIL: battery voltage fallback regression test failed'
    failed=1
fi

exit "$failed"
