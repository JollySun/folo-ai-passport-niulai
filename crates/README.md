# Shared Rust crates

`crates/` 只包含经过真实复用验证、且不带任何应用名称或策略的 Rust 模块。

| Crate | 职责 | 硬件依赖 |
| --- | --- | --- |
| [passport-core](passport-core/README.md) | PCM 和电池等纯计算 | 无，`no_std` |
| [passport-platform](passport-platform/README.md) | BSP/ESP-IDF 的安全 Rust facade | 通过 C FFI |

只有第二个真实应用出现相同语义后，才把代码从 `apps/<app>` 提取到这里。公共
crate 不得依赖应用 crate，也不得包含应用名称、资源、页面、任务或持久化策略。
