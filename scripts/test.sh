#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/niulai-tests.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

compiler=${CC:-cc}

run_test() {
    name=$1
    include_dir=$2
    test_source=$3
    source_file=$4

    "$compiler" -std=c11 -Wall -Wextra -Werror \
        -I"$project_dir/$include_dir" \
        "$project_dir/$test_source" "$project_dir/$source_file" \
        -o "$test_dir/$name"
    "$test_dir/$name"
    printf 'PASS: %s\n' "$name"
}

run_test niulai_model main \
    tests/test_niulai_model.c main/niulai_model.c
run_test niulai_audio_math main \
    tests/test_niulai_audio_math.c main/niulai_audio_math.c
run_test bsp_battery_math components/bsp/include \
    tests/test_bsp_battery_math.c components/bsp/src/bsp_battery_math.c

"$compiler" -std=gnu11 -Wall -Wextra -Werror \
    -I"$project_dir/tests/fakes" \
    -I"$project_dir/main" \
    "$project_dir/tests/test_niulai_ui_transition.c" \
    "$project_dir/main/niulai_app.c" \
    "$project_dir/main/niulai_model.c" \
    "$project_dir/main/niulai_audio_math.c" \
    -o "$test_dir/niulai_ui_transition"
"$test_dir/niulai_ui_transition"
printf 'PASS: %s\n' niulai_ui_transition

"$compiler" -std=c11 -Wall -Wextra -Werror \
    -I"$project_dir/tests/fakes" \
    -I"$project_dir/main" \
    "$project_dir/tests/test_niulai_font_glyphs.c" \
    "$project_dir/main/niulai_font_12.c" \
    -o "$test_dir/niulai_font_glyphs"
"$test_dir/niulai_font_glyphs"
printf 'PASS: %s\n' niulai_font_glyphs

printf 'All host tests passed.\n'
