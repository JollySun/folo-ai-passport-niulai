# Contributing

Thanks for improving the Niu Lai firmware. Keep changes focused, reproducible, and safe for the resource-constrained ESP32-C3 target.

## Before opening a change

1. Search existing issues and describe user-visible behavior before implementation details.
2. Read [Architecture](docs/ARCHITECTURE.md) and [Hardware](docs/HARDWARE.md) for the affected area.
3. Do not add movie or third-party media unless its redistribution terms are documented.
4. Keep pure reusable algorithms in `crates/passport-core`, reusable Rust hardware
   access in `crates/passport-platform`, reusable board mechanisms in
   `components/bsp_*`, and all app-specific behavior and resources in `apps/<app>`.
5. Follow [Adding an application](docs/ADDING_APPS.md) for new tools. A new app
   must be discoverable without adding its name to shared crates, components,
   CMake files, scripts, or workflows.

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
- Application behavior is Rust. C under `components/` must remain app-neutral;
  C files under an app firmware directory are limited to generated resources with
  their generation source recorded.
- Keep hardware constants in `components/bsp_board/include/bsp_pins.h`.
- Treat button callbacks as `esp_timer` callbacks: keep them bounded and never perform audio, Flash, or other long-running work in them.
- Every `Ui`/LVGL call must either run in the LVGL task or be enclosed by `display::lock()`/`bsp_lvgl_lock()`; application state methods must not contain hidden UI writes.
- When both locks are needed, acquire the display lock before the application-state mutex. Never wait for the display lock while holding application state.
- Prefer tests at a module interface over checks against implementation text.
- Update user documentation and `CHANGELOG.md` when behavior changes.

For any UI, callback, task, mutex, or FFI change, record these hardware checks in the pull request:

1. Monitor the 115200-baud serial log for at least three minutes, which exceeds the known watchdog regression window.
2. Repeatedly exercise short, long, release, and double-button events where applicable.
3. Exercise playback, animation, recording, saving, reset, and battery refresh paths affected by the change.
4. Confirm that the log contains no `task_wdt`, LVGL assertion, panic, or reboot.

See the [LVGL cross-task freeze incident](docs/incidents/2026-08-25-lvgl-cross-task-freeze.md) for why these checks are required.

## Commits and pull requests

Use focused Conventional Commit subjects such as `feat(app): ...`, `fix(bsp): ...`, `test: ...`, and `docs: ...`. A pull request should explain what changed, why, automated checks, hardware checks, compatibility impact, and any unverified behavior.

By contributing code, you agree that it may be distributed under this repository's MIT license. Do not submit content you do not have the right to license.
