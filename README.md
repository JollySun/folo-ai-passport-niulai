# Niu Lai for FoloToy AI Passport

English | [简体中文](README.zh_CN.md)

[![CI](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml/badge.svg)](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/code%20license-MIT-green.svg)](LICENSE)

An offline interactive “Niu Lai” soundboard for the ESP32-C3-based FoloToy AI Passport. It combines character animation, dialogue playback, battery status, volume control, and user-recorded replacement voices in one small firmware image.

<p align="center">
  <img src="assets/niulai/home-ui-preview.png" alt="Redesigned Niu Lai home screen" width="300">
</p>

> [!IMPORTANT]
> This is an unofficial fan prototype. The MIT license covers the source code only. Movie images and audio may have separate rights; review [the asset source and distribution notice](assets/niulai/SOURCES.md) before redistributing firmware or media.

## Features

- Full-screen poster and two-frame Niu Lai/Mama animations.
- Built-in 16 kHz mono dialogue with software volume control.
- Up to 10 seconds of replacement voice recording per character.
- Power-loss-safe, dual-bank recording storage in a dedicated flash partition.
- iOS-style battery indicator with a voltage fallback when CW2017 SOC is unavailable.
- Three-button UI; no network or account is required.

## Controls

| Input | Context | Action |
| --- | --- | --- |
| Short `UP` | Any non-settings page | Show Niu Lai and play “Mama~~” |
| Short `DOWN` | Any non-settings page | Show Mama and play “Niu Lai!” |
| Hold `UP`, then release | Niu Lai page | Record and save the Niu Lai-page voice |
| Hold `DOWN`, then release | Mama page | Record and save the Mama-page voice |
| Short `OK` | Any page | Open settings, or return from settings |
| Double `OK` | Settings | Delete custom recordings and restore built-in voices |
| Hold `OK` | Any page | Stop audio and return home |

See the [user guide](docs/USER_GUIDE.md) for installation, recording behavior, and troubleshooting.

## Install a release

Download the newest artifacts from [GitHub Releases](https://github.com/JollySun/folo-ai-passport-niulai/releases/latest).

- Flash `*-full.bin` at address `0x0` for the first installation or whenever the partition table changes.
- Flash `*-app.bin` at address `0x10000` only when the device already has this project's partition table.
- Verify downloads with `SHA256SUMS`.

The recording feature depends on the `recordings` partition. An app-only image cannot create it.

## Build from source

Requirements: FoloToy AI Passport, USB data cable, Git, Rustup, and ESP-IDF 5.5.3.

```bash
git clone https://github.com/JollySun/folo-ai-passport-niulai.git
cd folo-ai-passport-niulai
source "$HOME/esp/esp-idf/export.sh"
scripts/test.sh
idf.py build
idf.py flash monitor
```

Detailed setup, packaging, and flashing commands are in [Building](docs/BUILDING.md).

## Repository layout

```text
crates/passport-core/  Shared no_std signal and battery calculations
apps/niulai/core/      Niu Lai-specific no_std state model
components/passport-rust/ Temporary C ABI for the Rust migration
components/bsp/       Board drivers and hardware constants
main/                 UI, audio orchestration, and recording storage
main/assets/niulai/   Firmware-ready RGB565 and PCM assets
assets/niulai/        Human-viewable previews and provenance
tests/                C ABI behavior tests for hardware-independent logic
scripts/              Test and firmware-packaging entry points
docs/                 User, build, architecture, and hardware documentation
```

The application keeps shared calculations in the `passport-core` Rust crate,
Niu Lai state in `niulai-core`, persistent voice storage in
`niulai_voice_store`, and board access behind the BSP interfaces. See
[Architecture](docs/ARCHITECTURE.md) for the runtime and flash layout.

## Development

```bash
scripts/test.sh
idf.py build
scripts/package-firmware.sh build dist dev
```

Pull requests run host tests and a complete ESP-IDF build. Tags matching `v*` create a GitHub Release with full, app-only, bootloader, partition-table, and checksum artifacts.

Read [Contributing](CONTRIBUTING.md), [Security](SECURITY.md), and the [changelog](CHANGELOG.md) before submitting a change.

## License and acknowledgments

Source code is licensed under the [MIT License](LICENSE). Media assets are not automatically covered by that license; see [SOURCES.md](assets/niulai/SOURCES.md).

Built for the [FoloToy AI Passport](https://github.com/FoloToy/ai-passport) using [ESP-IDF](https://github.com/espressif/esp-idf), LVGL, `esp_lvgl_port`, `button`, and `esp_codec_dev`.
