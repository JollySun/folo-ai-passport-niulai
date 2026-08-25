# passport-core

硬件无关的 `no_std` Rust 计算 crate，可在主机直接测试。

当前 interface：

- `passport_core::scale_pcm16`：按限制在 0–100% 的音量缩放 PCM16 样本。
- `passport_core::battery_percent_from_voltage`：把电池毫伏值映射为近似百分比。

本 crate 不允许依赖 ESP-IDF、BSP、应用 crate、动态分配或应用资源。新增逻辑
必须至少有两个真实调用方，并通过公开 interface 的行为测试验证。

```bash
cargo test --locked --package passport-core
```
