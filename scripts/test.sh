#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/niulai-tests.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

compiler=${CC:-cc}
rust_target_dir="$test_dir/cargo-target"

cd "$project_dir"
cargo fmt --all --check
cargo test --locked --workspace --target-dir "$rust_target_dir"
cargo build --locked --package passport-rust --target-dir "$rust_target_dir"
rust_library="$rust_target_dir/debug/libpassport_rust.a"

run_test() {
    name=$1
    include_dir=$2
    test_source=$3

    "$compiler" -std=c11 -Wall -Wextra -Werror \
        -I"$project_dir/$include_dir" \
        "$project_dir/$test_source" "$rust_library" \
        -o "$test_dir/$name"
    "$test_dir/$name"
    printf 'PASS: %s\n' "$name"
}

run_test niulai_model main tests/test_niulai_model.c
run_test niulai_audio_math main tests/test_niulai_audio_math.c
run_test bsp_battery_math components/bsp/include tests/test_bsp_battery_math.c

printf 'All host tests passed.\n'
