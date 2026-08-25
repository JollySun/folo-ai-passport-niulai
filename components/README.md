# Shared C board modules

`components/` 提供与应用无关的 ESP-IDF 和板级机制。Rust 应用通常通过
[`passport-platform`](../crates/passport-platform/README.md) 使用这些能力，
不直接依赖 C 头文件。

| Module | 职责 | 主要依赖 |
| --- | --- | --- |
| `bsp_board` | 引脚和板级常量 | 无 |
| `bsp_runtime` | FreeRTOS task、queue、mutex、延时、日志 | ESP-IDF runtime |
| `bsp_i2c` | 唯一共享 I2C bus | `bsp_board` |
| `bsp_battery` | CW2017 访问 | `bsp_board`, `bsp_i2c` |
| `bsp_display` | LCD panel 和背光 | `bsp_board` |
| `bsp_lvgl` | LVGL port、锁和基础对象操作 | `bsp_display` |
| `bsp_button` | ADC 三键轮询和事件 | `bsp_board` |
| `bsp_audio` | ES8311 和 I2S | `bsp_board`, `bsp_i2c` |
| `bsp_storage` | ESP-IDF partition 原始读写 | ESP-IDF partition |
| `rust_app_entry` | `app_main` → `passport_app_main` | 所选 Rust 应用 |

硬件常量只在 `bsp_board/include/bsp_pins.h` 定义。C 模块不得包含应用名称、
应用状态或应用资源；应用 firmware 通过 `REQUIRES` 只选择实际使用的能力。

板级事实和实机验收要求见[硬件文档](../docs/architecture/hardware.md)。
