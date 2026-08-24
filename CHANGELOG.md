# Changelog

All notable changes follow [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed

- Began the Rust migration with `no_std` crates for shared calculations and the
  Niu Lai application model while retaining the proven ESP-IDF BSP, LVGL UI,
  and audio stack.
- Moved the complete Niu Lai application, C ABI, tests, and media under
  `apps/niulai`; application selection and releases are now independent of the
  shared Rust and BSP modules.
- Added a minimal independently released diagnostics application that reuses
  only the shared Rust core, I2C, and battery modules.
- Split application sdkconfig/partition policy from board defaults and divided
  the BSP build into independently selectable hardware modules.
- Moved the shared PCM and battery C interface into `passport-core`.

## [0.0.2] - 2026-08-24

### Changed

- Redesigned all four on-device pages with a cleaner layout, a unified modern palette, and fully Chinese UI copy.
- Replaced the three-glyph fonts with minimal 12/16/22 px Source Han Sans SC subsets for the complete interface.
- Replaced the generic hardware-demo documentation with project-specific user, build, architecture, and hardware guides.
- Consolidated host tests and firmware packaging behind reusable scripts.
- Added separate CI and tag-driven release workflows plus repository contribution templates.
- Removed unused legacy demo UI code and an unembedded animation frame.
- Committed the ESP-IDF dependency lock for reproducible builds.

### Fixed

- Resetting custom voices now erases the complete recording partition instead of leaving recoverable PCM data behind invalid headers.

## [0.0.1] - 2026-08-24

### Added

- Initial open-source release of the Niu Lai interactive firmware.
- Versioned full-device and app-only CI artifacts.

[Unreleased]: https://github.com/JollySun/folo-ai-passport-niulai/compare/v0.0.2...HEAD
[0.0.2]: https://github.com/JollySun/folo-ai-passport-niulai/compare/v0.0.1...v0.0.2
[0.0.1]: https://github.com/JollySun/folo-ai-passport-niulai/releases/tag/v0.0.1
