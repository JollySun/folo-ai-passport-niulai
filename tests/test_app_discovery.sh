#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/passport-app-discovery.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

mkdir -p "$test_dir/scripts" "$test_dir/apps/valid-app/firmware"
cp "$project_dir/scripts/list-apps.sh" "$test_dir/scripts/list-apps.sh"
chmod +x "$test_dir/scripts/list-apps.sh"

touch "$test_dir/apps/valid-app/Cargo.toml"
touch "$test_dir/apps/valid-app/firmware/CMakeLists.txt"
touch "$test_dir/apps/valid-app/sdkconfig.defaults"
touch "$test_dir/apps/valid-app/test.sh"
chmod +x "$test_dir/apps/valid-app/test.sh"

if [ "$("$test_dir/scripts/list-apps.sh")" != "valid-app" ]; then
    printf 'FAIL: valid application was not discovered\n' >&2
    exit 1
fi
if [ "$("$test_dir/scripts/list-apps.sh" --json)" != '["valid-app"]' ]; then
    printf 'FAIL: application JSON output is invalid\n' >&2
    exit 1
fi

mkdir -p "$test_dir/apps/zz-incomplete-app"
if "$test_dir/scripts/list-apps.sh" >"$test_dir/output" 2>"$test_dir/error"; then
    printf 'FAIL: incomplete application was silently ignored\n' >&2
    exit 1
fi
if [ -s "$test_dir/output" ]; then
    printf 'FAIL: incomplete application produced a partial result\n' >&2
    exit 1
fi
if ! grep -Fq 'missing apps/zz-incomplete-app/Cargo.toml' "$test_dir/error"; then
    printf 'FAIL: incomplete application error is not actionable\n' >&2
    exit 1
fi

rm -rf "$test_dir/apps/zz-incomplete-app"
chmod -x "$test_dir/apps/valid-app/test.sh"
if "$test_dir/scripts/list-apps.sh" >"$test_dir/output" 2>"$test_dir/error"; then
    printf 'FAIL: non-executable application test was accepted\n' >&2
    exit 1
fi
if ! grep -Fq 'apps/valid-app/test.sh is not executable' "$test_dir/error"; then
    printf 'FAIL: non-executable test error is not actionable\n' >&2
    exit 1
fi
chmod +x "$test_dir/apps/valid-app/test.sh"

mkdir -p "$test_dir/apps/_invalid"
if "$test_dir/scripts/list-apps.sh" >"$test_dir/output" 2>"$test_dir/error"; then
    printf 'FAIL: invalid application name was accepted\n' >&2
    exit 1
fi
if ! grep -Fq 'Invalid application directory name: _invalid' "$test_dir/error"; then
    printf 'FAIL: invalid application name error is not actionable\n' >&2
    exit 1
fi

printf 'PASS: app discovery contract\n'
