## What changed

Describe the user-visible behavior and the reason for the change.

## Verification

- [ ] `scripts/test.sh`
- [ ] `scripts/build-app.sh <app> build`
- [ ] Tested on a FoloToy AI Passport, or hardware testing is not required

Hardware revision and observations:

For UI, callback, task, mutex, or FFI changes, include the serial-monitor duration
and confirm whether `task_wdt`, LVGL assertions, panics, or reboots occurred.

## Checklist

- [ ] No generated build output or unlicensed media was added
- [ ] App-specific behavior and resources remain under `apps/<app>`
- [ ] Shared crates, components, CMake, and scripts remain app-neutral
- [ ] Owning documentation and the applicable workspace/app changelog were updated
- [ ] Display, audio, battery, storage, and button regressions were considered
