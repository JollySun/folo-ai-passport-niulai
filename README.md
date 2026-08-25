# FoloToy AI Passport Apps

English | [简体中文](README.zh_CN.md)

[![CI](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml/badge.svg)](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/code%20license-MIT-green.svg)](LICENSE)

A monorepo for independently buildable and releasable ESP32-C3 tools targeting
the FoloToy AI Passport. Application behavior is implemented in Rust; shared C
code is limited to app-neutral board and ESP-IDF mechanisms exposed through a
Rust facade.

## Applications

| Application | Purpose | Build | Release tag |
| --- | --- | --- | --- |
| [Niu Lai](apps/niulai/README.md) | Offline animation, dialogue playback, recording, and battery UI | `scripts/build-app.sh niulai build` | `niulai/v<version>` |
| [Diagnostics](apps/diagnostics/README.md) | Minimal I2C and battery-gauge diagnostics | `scripts/build-app.sh diagnostics build` | `diagnostics/v<version>` |

Each application owns its Rust crate, firmware metadata, tests, configuration,
partition policy, and resources under `apps/<app>`. The shared workspace does
not contain an application registry or application-specific switches.

## Workspace

```text
apps/                      Independent tools and their resources
crates/                    Reusable Rust modules
  passport-core/           Hardware-independent no_std calculations
  passport-platform/       Rust facade over BSP and ESP-IDF capabilities
components/                App-neutral C board mechanisms and stable Rust entry
cmake/                     Shared Rust static-library build integration
scripts/                   Discovery, test, build, and packaging entry points
docs/                      Cross-application architecture and development docs
```

The stable C `app_main` calls `passport_app_main` from the selected Rust
application. Adding a valid `apps/<app>` directory does not require edits to the
root Cargo workspace, root CMake project, CI matrix, or release workflow. See
the [architecture](docs/architecture/overview.md) for the complete dependency
and runtime model.

## Quick start

Use ESP-IDF 5.5.3 and the Rust toolchain pinned by `rust-toolchain.toml`:

```bash
scripts/list-apps.sh
scripts/test.sh
scripts/build-app.sh niulai build
scripts/build-app.sh niulai flash monitor
```

Build output is isolated under `build/<app>`. Tags matching `<app>/v*` build and
publish only that application.

## Documentation

- [Documentation index](docs/README.md)
- [Monorepo architecture](docs/architecture/overview.md)
- [Build, flash, and release](docs/development/building.md)
- [Add an application](docs/development/adding-apps.md)
- [Board hardware](docs/architecture/hardware.md)
- [Contributing](CONTRIBUTING.md)
- [Workspace changelog](CHANGELOG.md)

Application usage and resource notices live with the application, not in the
cross-application documentation tree.

## License

Source code is licensed under the [MIT License](LICENSE). Application media may
have different redistribution terms; review each application's asset notices
before distributing firmware or media.
