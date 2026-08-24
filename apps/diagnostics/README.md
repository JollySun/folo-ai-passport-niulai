# Board diagnostics

This minimal firmware scans the shared I2C bus, probes the CW2017 battery
gauge, and logs its voltage, reported SOC, and the shared Rust voltage
estimate. It intentionally does not depend on display, LVGL, button, or audio
modules.

```bash
scripts/build-app.sh diagnostics build
scripts/build-app.sh diagnostics flash monitor
```

Release it independently with a tag such as `diagnostics/v1.0.0`.
