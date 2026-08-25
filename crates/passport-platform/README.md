# passport-platform

面向 Rust 应用的公共硬件 facade。它集中声明 C FFI，并用 Rust 类型、结果和
RAII guard 表达资源生命周期；应用不应重复声明 BSP FFI。

| Rust module | 能力 | C module |
| --- | --- | --- |
| `runtime`, `log` | task、queue、mutex、延时和日志 | `bsp_runtime` |
| `i2c` | 共享总线初始化和扫描 | `bsp_i2c` |
| `battery` | CW2017 电量与电压降级 | `bsp_battery` |
| `display` | LCD/LVGL 初始化、背光和显示锁 | `bsp_display`, `bsp_lvgl` |
| `ui` | LVGL 对象的 Rust 表达 | `bsp_lvgl` |
| `button` | 三键事件 | `bsp_button` |
| `audio` | codec/I2S 播放与录音 | `bsp_audio` |
| `storage` | 原始 Flash 分区读写 | `bsp_storage` |

调用方仍必须遵守底层执行上下文：FFI callback 不会因为进入 Rust 而切换任务；
LVGL task 之外的 UI 访问必须持有 display lock。应用专属重试、页面、录音协议
和分区格式不属于本 crate。

```bash
cargo test --locked --package passport-platform
```
