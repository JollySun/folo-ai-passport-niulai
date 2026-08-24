# Repository Guidelines

## Structure

- `main/`: Niu Lai state model, LVGL UI, audio orchestration, fonts, and persistent voice storage.
- `components/bsp/`: reusable display, button, audio, battery, and shared-I2C modules.
- `components/bsp/include/bsp_pins.h`: single source of truth for pins and board constants.
- `tests/`: host-side tests for hardware-independent module interfaces.
- `scripts/`: stable test and packaging entry points used locally and in CI.
- `docs/`: user, build, architecture, and hardware documentation.

Keep application behavior in `main` and reusable board access in `components/bsp`. Do not add an interface or adapter unless behavior really varies across that seam.

## Commands

Use ESP-IDF 5.5.3:

```bash
scripts/test.sh
idf.py build
idf.py flash monitor
scripts/package-firmware.sh build dist dev
```

Treat host tests and a clean build as the minimum automated checks. Display, buttons, audio, recording, battery, and timing conclusions require physical-device validation.

## Code and tests

Use four-space C indentation, K&R braces, `snake_case`, `s_` for file-local state, and `bsp_` for BSP interfaces. Keep blocking work out of button callbacks and lock LVGL outside its task. Preserve comments that explain hardware register values, memory limits, and initialization order.

Test observable behavior through module interfaces. Do not replace behavioral tests with source-text matching. Do not edit `managed_components/`, generated fonts, or binary assets manually.

## Changes

Preserve unrelated worktree changes. Commit messages use focused Conventional Commit subjects. Pull requests must record automated checks, applicable hardware results, and unverified items. New media requires a source and redistribution notice; the MIT license covers code, not third-party movie assets.
