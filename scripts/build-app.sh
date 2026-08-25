#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
app=${1:-}
if [ -z "$app" ]; then
    printf 'Usage: %s <app> <idf.py command...>\n' "$0" >&2
    exit 2
fi
shift

case "$app" in
    [a-z0-9]*) ;;
    *)
        printf 'Invalid application name: %s\n' "$app" >&2
        exit 2
        ;;
esac
case "$app" in
    *[!a-z0-9_-]*)
        printf 'Invalid application name: %s\n' "$app" >&2
        exit 2
        ;;
esac
if [ ! -f "$project_dir/apps/$app/firmware/CMakeLists.txt" ]; then
    printf 'Unknown application: %s\nAvailable applications:\n' "$app" >&2
    "$project_dir/scripts/list-apps.sh" >&2
    exit 2
fi
app_defaults="$project_dir/apps/$app/sdkconfig.defaults"
if [ ! -f "$app_defaults" ]; then
    printf 'Application %s has no sdkconfig.defaults\n' "$app" >&2
    exit 2
fi

exec idf.py \
    -B "$project_dir/build/$app" \
    -D PASSPORT_APP="$app" \
    -D SDKCONFIG="$project_dir/build/$app/sdkconfig" \
    -D "SDKCONFIG_DEFAULTS=$project_dir/sdkconfig.defaults;$app_defaults" \
    "$@"
