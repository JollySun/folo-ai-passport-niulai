#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

# Applications may contain generated C resources, but handwritten application
# behaviour belongs in Rust.
for source in "$project_dir"/apps/*/firmware/*.[ch]; do
    [ -f "$source" ] || continue
    case "${source##*/}" in
        *_font_*.c)
            if ! grep -q "Generated from" "$source"; then
                printf 'Unmarked C application source: %s\n' "$source" >&2
                exit 1
            fi
            ;;
        *)
            printf 'Handwritten application C is not allowed: %s\n' "$source" >&2
            exit 1
            ;;
    esac
done

# Reusable modules must not know application names. Applications are selected
# by directory discovery and the stable passport_app_main symbol.
for app in $("$project_dir/scripts/list-apps.sh"); do
    if grep -R -i -n --exclude-dir=target --exclude='Cargo.lock' \
        "$app" \
        "$project_dir/crates" "$project_dir/components" "$project_dir/cmake" \
        "$project_dir/CMakeLists.txt" "$project_dir/Cargo.toml" \
        "$project_dir/scripts" >/dev/null; then
        printf 'Reusable code contains application name: %s\n' "$app" >&2
        exit 1
    fi
done

printf 'Architecture contract passed.\n'
