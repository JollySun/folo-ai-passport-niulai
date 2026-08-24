#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_dir=${1:?test output directory is required}
rust_target_dir="$test_dir/cargo-target"

mkdir -p "$test_dir"
cd "$project_dir"
cargo test --locked --package diagnostics-app --target-dir "$rust_target_dir"
printf 'PASS: diagnostics Rust application\n'
