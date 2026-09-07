# Niu Lai for FoloToy AI Passport

English | [简体中文](README.zh_CN.md)

[![CI](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml/badge.svg)](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/code%20license-MIT-green.svg)](LICENSE)

An offline interactive “Niu Lai” soundboard for the ESP32-C3-based FoloToy AI Passport. It combines character animation, dialogue playback, battery status, volume control, and user-recorded replacement voices in one small firmware image.

<p align="center">
  <img src="docs/screenshots/home.png" alt="Niu Lai home screen" width="300">
</p>

> [!IMPORTANT]
> This is an unofficial fan prototype. The MIT license covers the source code only. Movie images and audio may have separate rights; review [the asset source and distribution notice](assets/niulai/SOURCES.md) before redistributing firmware or media.

## Screen captures

These captures are rendered from the current firmware's 240×320 LVGL layout and
the embedded character frames. The LCD has a write-only SPI connection, so these
are software UI captures for documentation rather than camera photos of the
physical panel.

<p align="center">
  <img src="docs/screenshots/home.png" alt="Home screen" width="180">
  <img src="docs/screenshots/niulai.png" alt="Niu Lai page" width="180">
</p>
<p align="center">
  <img src="docs/screenshots/mama.png" alt="Mama page" width="180">
  <img src="docs/screenshots/settings.png" alt="Settings page" width="180">
</p>

## Features

- Full-screen poster and two-frame Niu Lai/Mama animations.
- Built-in dialogue with adjustable playback volume.
- Up to 10 seconds of replacement voice recording per character.
- Saved recordings survive restarts, and an interrupted save keeps the previous recording intact.
- Custom-voice animations stop when the recorded voice finishes.
- iOS-style battery indicator with a voltage fallback when CW2017 SOC is unavailable.
- Three-button UI; no network or account is required.

## Controls

| Input | Context | Action |
| --- | --- | --- |
| Short `UP` | Any non-settings page | Show Niu Lai and play “Mama~~” |
| Short `DOWN` | Any non-settings page | Show Mama and play “Niu Lai!” |
| Repeat `UP` or `DOWN` within 1.8 s | Any non-settings page | Intensify that character's caption and animation |
| Alternate `UP` and `DOWN` within 1.8 s | Any non-settings page | Flash both character colors as a call-and-response |
| Hold `UP`, then release | Niu Lai page | Record and save the Niu Lai-page voice |
| Hold `DOWN`, then release | Mama page | Record and save the Mama-page voice |
| Short `OK` | Any non-settings page | Open settings |
| Short `OK` | Settings | Activate/deactivate the selected value, or confirm voice reset |
| Short `UP` or `DOWN` | Voice-reset confirmation | Cancel the reset |
| Hold `OK` | Settings | Return to the page that opened settings |
| Hold `OK` | Any other page | Stop audio and return home |

The settings footer always shows the current button actions and a second-line
`Hold OK to return` reminder.

After 30 seconds without input, the LCD panel and audio path enter standby without changing the current caption. The next complete button gesture wakes the display without triggering its normal action. If the device remains idle for 5 minutes, the ESP32-C3 enters deep sleep; a low-level press on GPIO0 wakes it by rebooting the app.

See the [user guide](docs/USER_GUIDE.md) for installation, recording behavior, and troubleshooting.

## Install a release

Download the newest artifacts from [GitHub Releases](https://github.com/JollySun/folo-ai-passport-niulai/releases/latest).

- Flash `*-full.bin` at address `0x0` for the first installation or whenever the partition table changes.
- Flash `*-app.bin` at address `0x10000` only when the device already has this project's partition table.
- Verify downloads with `SHA256SUMS`.

If recording is unavailable after an app-only update, reinstall with the full image.

## Build from source

Requirements: FoloToy AI Passport, USB data cable, Git, and ESP-IDF 5.5.3.

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
components/bsp/       Board drivers and hardware constants
main/                 Application state, UI, audio, and recording storage
main/assets/niulai/   Firmware-ready RGB565 and PCM assets
assets/niulai/        Human-viewable previews and provenance
tests/                Hardware-independent C tests
scripts/              Test and firmware-packaging entry points
docs/                 User, build, architecture, and hardware documentation
```

The application keeps hardware-independent state in `niulai_model`, persistent voice storage in `niulai_voice_store`, and board access behind the BSP interfaces. See [Architecture](docs/ARCHITECTURE.md) for the runtime and flash layout.

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
