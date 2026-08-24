# Repository Guidelines

## Structure

- `apps/`: tool-specific Rust state and, as migration proceeds, independent firmware crates.
- `crates/passport-core/`: hardware-independent Rust logic shared by multiple tools.
- `main/`: current Niu Lai LVGL UI, audio orchestration, fonts, and persistent voice storage.
- `components/passport-rust/`: temporary C ABI adapter used during the Rust migration.
- `components/bsp/`: reusable display, button, audio, battery, and shared-I2C modules.
- `components/bsp/include/bsp_pins.h`: single source of truth for pins and board constants.
- `tests/`: host-side tests for hardware-independent module interfaces.
- `scripts/`: stable test and packaging entry points used locally and in CI.
- `docs/`: user, build, architecture, and hardware documentation.

Keep tool-specific behavior under its `apps/` directory, current C orchestration in
`main`, reusable pure logic in `crates/passport-core`, and reusable board access
in `components/bsp`. Do not add an interface or adapter unless behavior really
varies across that seam; migration adapters must document their deletion condition.

## Commands

Use ESP-IDF 5.5.3 and the Rust toolchain pinned by `rust-toolchain.toml`:

```bash
scripts/test.sh
idf.py build
idf.py flash monitor
scripts/package-firmware.sh build dist dev
```

Treat host tests and a clean build as the minimum automated checks. Display, buttons, audio, recording, battery, and timing conclusions require physical-device validation.

## Code and tests

Use four-space C indentation, K&R braces, `snake_case`, `s_` for file-local state,
and `bsp_` for BSP interfaces. Format Rust with `rustfmt`; keep shared core crates
`no_std` and hardware-independent. Keep blocking work out of button callbacks and
lock LVGL outside its task. Preserve comments that explain hardware register values,
memory limits, and initialization order.

Test observable behavior through module interfaces. Do not replace behavioral tests with source-text matching. Do not edit `managed_components/`, generated fonts, or binary assets manually.

## Changes

Preserve unrelated worktree changes. Commit messages use focused Conventional Commit subjects. Pull requests must record automated checks, applicable hardware results, and unverified items. New media requires a source and redistribution notice; the MIT license covers code, not third-party movie assets.
