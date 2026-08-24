#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
format=${1:-lines}
first=true

if [ "$format" = "--json" ]; then
    printf '['
elif [ "$format" != "lines" ]; then
    printf 'Usage: %s [--json]\n' "$0" >&2
    exit 2
fi

for manifest in "$project_dir"/apps/*/Cargo.toml; do
    [ -f "$manifest" ] || continue
    app_dir=${manifest%/Cargo.toml}
    [ -f "$app_dir/firmware/CMakeLists.txt" ] || continue
    [ -x "$app_dir/test.sh" ] || continue
    app=${app_dir##*/}

    if [ "$format" = "--json" ]; then
        if [ "$first" = false ]; then
            printf ','
        fi
        printf '"%s"' "$app"
        first=false
    else
        printf '%s\n' "$app"
    fi
done

if [ "$format" = "--json" ]; then
    printf ']\n'
fi
