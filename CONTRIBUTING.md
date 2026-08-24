# Contributing

Thanks for improving the Niu Lai firmware. Keep changes focused, reproducible, and safe for the resource-constrained ESP32-C3 target.

## Before opening a change

1. Search existing issues and describe user-visible behavior before implementation details.
2. Read [Architecture](docs/ARCHITECTURE.md) and [Hardware](docs/HARDWARE.md) for the affected area.
3. Do not add movie or third-party media unless its redistribution terms are documented.
4. Keep pure reusable behavior in `crates/passport-core`, all Niu Lai code and
   resources in `apps/niulai`, and reusable board access in `components/bsp_*`.

## Local verification

Use ESP-IDF 5.5.3 and run:

```bash
scripts/test.sh
scripts/build-app.sh niulai build
git diff --check
```

Hardware-facing changes must also be tested on a FoloToy AI Passport. Record the board revision if known, serial-log result, and relevant display/button/audio/battery observations in the pull request.

## Style

- C uses four-space indentation, K&R braces, `snake_case`, `s_` for file-local state, and `bsp_` for BSP interfaces.
- Rust is formatted with `rustfmt`; keep `passport-core` free of ESP-IDF and allocation unless a measured requirement justifies it.
- Keep hardware constants in `components/bsp_board/include/bsp_pins.h`.
- Keep blocking I/O out of button callbacks and protect cross-task LVGL access with `bsp_lvgl_lock()`.
- Prefer tests at a module interface over checks against implementation text.
- Update user documentation and `CHANGELOG.md` when behavior changes.

## Commits and pull requests

Use focused Conventional Commit subjects such as `feat(app): ...`, `fix(bsp): ...`, `test: ...`, and `docs: ...`. A pull request should explain what changed, why, automated checks, hardware checks, compatibility impact, and any unverified behavior.

By contributing code, you agree that it may be distributed under this repository's MIT license. Do not submit content you do not have the right to license.
