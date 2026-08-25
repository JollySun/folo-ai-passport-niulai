#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
format=${1:-lines}
apps=

if [ "$format" != "--json" ] && [ "$format" != "lines" ]; then
    printf 'Usage: %s [--json]\n' "$0" >&2
    exit 2
fi

for app_dir in "$project_dir"/apps/*; do
    [ -d "$app_dir" ] || continue
    app=${app_dir##*/}

    case "$app" in
        [a-z0-9]*) ;;
        *)
            printf 'Invalid application directory name: %s\n' "$app" >&2
            exit 1
            ;;
    esac
    case "$app" in
        *[!a-z0-9_-]*)
            printf 'Invalid application directory name: %s\n' "$app" >&2
            exit 1
            ;;
    esac

    for required in Cargo.toml firmware/CMakeLists.txt sdkconfig.defaults test.sh; do
        if [ ! -f "$app_dir/$required" ]; then
            printf 'Incomplete application %s: missing apps/%s/%s\n' \
                "$app" "$app" "$required" >&2
            exit 1
        fi
    done
    if [ ! -x "$app_dir/test.sh" ]; then
        printf 'Incomplete application %s: apps/%s/test.sh is not executable\n' \
            "$app" "$app" >&2
        exit 1
    fi

    apps="${apps}${apps:+ }$app"
done

first=true
if [ "$format" = "--json" ]; then
    printf '['
fi
for app in $apps; do
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
