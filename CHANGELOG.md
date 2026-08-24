# Changelog

All notable changes follow [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

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

[Unreleased]: https://github.com/JollySun/folo-ai-passport-niulai/compare/v0.0.1...HEAD
[0.0.1]: https://github.com/JollySun/folo-ai-passport-niulai/releases/tag/v0.0.1
