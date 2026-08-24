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
cargo build --locked --package diagnostics-app --target-dir "$rust_target_dir"
rust_library="$rust_target_dir/debug/libdiagnostics_app.a"

"$compiler" -std=c11 -Wall -Wextra -Werror \
    -I"$app_dir/firmware" \
    "$app_dir/tests/test_diagnostics.c" "$rust_library" \
    -o "$test_dir/diagnostics"
"$test_dir/diagnostics"
printf 'PASS: diagnostics\n'

"$compiler" -std=c11 -Wall -Wextra -Werror \
    -I"$project_dir/crates/passport-core/include" \
    "$project_dir/tests/test_passport_core_ffi.c" "$rust_library" \
    -o "$test_dir/passport_core_ffi"
"$test_dir/passport_core_ffi"
printf 'PASS: passport_core_ffi (diagnostics archive)\n'
