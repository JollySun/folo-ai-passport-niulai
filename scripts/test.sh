#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/passport-tests.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

cd "$project_dir"
cargo fmt --all --check
cargo test --locked --workspace --target-dir "$test_dir/cargo-target"

${CC:-cc} -std=c11 -Wall -Wextra -Werror \
    -I"components/bsp_board/include" \
    tests/test_bsp_board_header.c \
    -o "$test_dir/bsp_board_header"
"$test_dir/bsp_board_header"
printf 'PASS: bsp_board_header\n'

for app in $(scripts/list-apps.sh); do
    "apps/$app/test.sh" "$test_dir/$app"
done

printf 'All host tests passed.\n'
