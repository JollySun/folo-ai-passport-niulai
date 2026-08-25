# Niu Lai changelog

All notable Niu Lai application changes follow
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed

- Completed the Rust migration for application state, UI orchestration, tasks,
  and voice-store policy while retaining app-neutral board mechanisms in C.
- Moved the complete application, tests, firmware metadata, and media under
  `apps/niulai` for independent builds and releases.

### Fixed

- Prevented button callbacks running in the `esp_timer` task from modifying
  LVGL objects before acquiring the display lock, which could trigger an
  `lv_inv_area()` assertion and leave the device in a watchdog loop.

## [0.0.2] - 2026-08-24

### Changed

- Redesigned all four on-device pages with a unified palette and Chinese UI copy.
- Added minimal 12/16/22 px Source Han Sans SC subsets for the interface.
- Added host tests, firmware packaging, CI, and versioned release artifacts.
- Removed unused legacy demo UI code and an unembedded animation frame.

### Fixed

- Resetting custom voices erases the complete recording partition instead of
  leaving recoverable PCM data behind invalid headers.

## [0.0.1] - 2026-08-24

### Added

- Initial open-source release of the Niu Lai interactive firmware.
- Versioned full-device and app-only CI artifacts.

[Unreleased]: https://github.com/JollySun/folo-ai-passport-niulai/compare/v0.0.2...HEAD
[0.0.2]: https://github.com/JollySun/folo-ai-passport-niulai/compare/v0.0.1...v0.0.2
[0.0.1]: https://github.com/JollySun/folo-ai-passport-niulai/releases/tag/v0.0.1
