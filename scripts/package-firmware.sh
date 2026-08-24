#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${1:-build}
output_dir=${2:-dist}
version=${3:-dev}

case "$version" in
    *[!A-Za-z0-9._-]*)
        printf 'Version may contain only letters, numbers, dot, underscore, and dash.\n' >&2
        exit 1
        ;;
esac

case "$build_dir" in
    /*) ;;
    *) build_dir="$project_dir/$build_dir" ;;
esac
case "$output_dir" in
    /*) ;;
    *) output_dir="$project_dir/$output_dir" ;;
esac

app_image="$build_dir/FoloToy-AI-Passport.bin"
bootloader_image="$build_dir/bootloader/bootloader.bin"
partition_image="$build_dir/partition_table/partition-table.bin"

for image in "$app_image" "$bootloader_image" "$partition_image"; do
    if [ ! -f "$image" ]; then
        printf 'Missing build artifact: %s\nRun idf.py build first.\n' "$image" >&2
        exit 1
    fi
done

mkdir -p "$output_dir"
prefix="folo-ai-passport-niulai-$version"

cd "$project_dir"
python -m esptool --chip esp32c3 merge_bin \
    --output "$output_dir/$prefix-full.bin" \
    --format raw \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 8MB \
    0x0 "$bootloader_image" \
    0x8000 "$partition_image" \
    0x10000 "$app_image"
cp "$app_image" "$output_dir/$prefix-app.bin"
cp "$bootloader_image" "$output_dir/$prefix-bootloader.bin"
cp "$partition_image" "$output_dir/$prefix-partition-table.bin"

(
    cd "$output_dir"
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$prefix"-*.bin > SHA256SUMS
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$prefix"-*.bin > SHA256SUMS
    else
        printf 'Neither sha256sum nor shasum is available.\n' >&2
        exit 1
    fi
)

printf 'Firmware package written to %s\n' "$output_dir"
