# Diagnostics changelog

All notable diagnostics application changes follow
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Added the independently buildable diagnostics application for shared I2C,
  CW2017 battery readings, and the common Rust voltage estimate.
- Kept the dependency set limited to runtime, I2C, battery, and
  `passport-core`, without display, LVGL, button, audio, or storage modules.
