#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

app_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$app_dir/../.." && pwd)
test_dir=${1:?test output directory is required}
compiler=${CC:-cc}
rust_target_dir="$test_dir/cargo-target"

mkdir -p "$test_dir"
cd "$project_dir"
cargo build --locked --package niulai-app --target-dir "$rust_target_dir"
rust_library="$rust_target_dir/debug/libniulai_app.a"

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

run_test niulai_model apps/niulai/firmware apps/niulai/tests/test_niulai_model.c
run_test passport_core_ffi crates/passport-core/include tests/test_passport_core_ffi.c
