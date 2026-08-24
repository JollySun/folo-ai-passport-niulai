# 新增独立应用

## 目标

每个小工具都是 `apps/<app>` 下的独立 Rust crate，可以单独测试、构建、烧录
和发布。新增应用不需要修改公共入口、根 Cargo workspace、根 CMake、CI matrix
或 release workflow。

应用名只能包含小写字母、数字、下划线和连字符，并以字母或数字开头。Cargo
package 名可以使用连字符；用于 static library 的名称使用下划线。

## 自动发现契约

以下文件必须同时存在，且 `test.sh` 必须可执行：

```text
apps/example/
├── Cargo.toml
├── sdkconfig.defaults
├── test.sh
├── src/
│   └── lib.rs
└── firmware/
    └── CMakeLists.txt
```

检查发现结果：

```bash
scripts/list-apps.sh
scripts/list-apps.sh --json
```

`scripts/check-architecture.sh` 还会拒绝公共 `crates/`、`components/`、`cmake/`、
根构建文件和 scripts 中出现应用名，并拒绝 app firmware 目录中的手写 C 行为
代码。

## 1. 创建 Rust crate

`apps/example/Cargo.toml`：

```toml
[package]
name = "example-app"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
license.workspace = true

[lib]
crate-type = ["staticlib"]
doctest = false

[dependencies]
passport-core = { path = "../../crates/passport-core" }
passport-platform = { path = "../../crates/passport-platform" }
```

只在确实使用共享纯函数时依赖 `passport-core`。所有 ESP-IDF、FreeRTOS 和板卡
能力都从 `passport-platform` 的 Rust interface 使用，不在应用中重复声明 C FFI。

`apps/example/src/lib.rs` 的最小入口：

```rust
// SPDX-License-Identifier: MIT

#![cfg_attr(target_os = "espidf", no_std)]

#[cfg(target_os = "espidf")]
#[no_mangle]
pub extern "C" fn passport_app_main() -> i32 {
    use passport_platform::log::{self, Level};

    log::write(Level::Info, c"example", c"Example application started");
    0
}

#[cfg(test)]
mod tests {
    #[test]
    fn host_logic_is_testable() {
        assert_eq!(2 + 2, 4);
    }
}
```

`passport_app_main` 是所有固件唯一需要实现的稳定入口。不要新增 app-specific
`app_main.c`、公共 header、入口 switch 或注册表。

## 2. 声明 firmware component

`apps/example/firmware/CMakeLists.txt`：

```cmake
idf_component_register(
    REQUIRES rust_app_entry bsp_runtime
)

include("${PROJECT_DIR}/cmake/passport_rust_app.cmake")
passport_add_rust_staticlib(PACKAGE example-app LIBRARY example_app)
```

在 `REQUIRES` 中只加入该应用实际使用的能力：

| Rust module | ESP-IDF component |
| --- | --- |
| `passport_platform::runtime`, `log` | `bsp_runtime` |
| `i2c` | `bsp_i2c` |
| `battery` | `bsp_battery` |
| `display` | `bsp_display bsp_lvgl` |
| `ui` | `bsp_lvgl lvgl__lvgl` |
| `button` | `bsp_button` |
| `audio` | `bsp_audio` |
| `storage` | `bsp_storage` |

始终保留 `rust_app_entry`。CMake 会解析 BSP 自身的传递依赖，但 firmware
component 应清楚列出应用直接使用的能力，方便审查固件组成。

## 3. 配置与分区

创建 `apps/example/sdkconfig.defaults`，只写应用专属配置。目标芯片、Flash 和
USB console 等板级默认值已经在根 `sdkconfig.defaults` 中。

不需要额外配置时可以保留只有注释的文件：

```text
# Example has no application-specific sdkconfig overrides.
```

需要自定义分区时，把 `partitions.csv` 放入应用 firmware 目录，并在该应用的
`sdkconfig.defaults` 中设置 `CONFIG_PARTITION_TABLE_CUSTOM` 和相对路径。不要把
一个应用的分区策略放入根配置。

## 4. 添加主机测试入口

`apps/example/test.sh`：

```sh
#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_dir=${1:?test output directory is required}
rust_target_dir="$test_dir/cargo-target"

mkdir -p "$test_dir"
cd "$project_dir"
cargo test --locked --package example-app --target-dir "$rust_target_dir"
printf 'PASS: example Rust application\n'
```

设置可执行权限，并让 Cargo 首次解析新 workspace member：

```bash
chmod +x apps/example/test.sh
cargo check --package example-app
scripts/test.sh
```

若 `Cargo.lock` 发生变化，应一并提交。测试应用可观察行为，不使用源码文本匹配
代替行为测试。

## 5. 添加资源

- 应用专属源码、状态、UI、持久化策略和资源全部放在 `apps/example`。
- 可人工查看的原始媒体放在 `apps/example/assets`，并记录来源和再分发许可。
- 直接嵌入固件的二进制放在 `apps/example/firmware/assets`，通过该应用
  `CMakeLists.txt` 的 `EMBED_FILES` 明确列出。
- 应用行为不能使用手写 C。当前架构检查只允许文件名匹配 `*_font_*.c` 且包含
  `Generated from` 来源标记的生成字体 C 文件。

不要把应用资源或名称放入 `crates/`、`components/` 或通用 scripts。

## 6. 构建和真机验证

```bash
scripts/build-app.sh example set-target esp32c3
scripts/build-app.sh example build
scripts/build-app.sh example flash monitor
```

应用只通过 `passport-platform` 使用硬件，但 callback 仍保留底层 C/ESP-IDF 的
执行上下文。使用 LVGL、button、task 或 mutex 前必须阅读
[LVGL 并发约束](ARCHITECTURE.md#lvgl-并发约束)，并按
[硬件说明](HARDWARE.md)和[贡献指南](../CONTRIBUTING.md#local-verification)完成实机验收。

## 7. 独立打包和发布

本地打包：

```bash
scripts/package-firmware.sh example build/example dist/example dev
```

发布标签：

```bash
git tag -a example/v1.0.0 -m "example v1.0.0"
git push origin example/v1.0.0
```

release workflow 从标签前缀解析应用，只构建 `example`，并创建该应用独立的
GitHub Release。发布文件包括 full、app、bootloader、partition-table 镜像和
`SHA256SUMS`。

## 完成检查

- `scripts/list-apps.sh` 只新增预期应用。
- `scripts/test.sh` 和 `git diff --check` 通过。
- `scripts/build-app.sh example build` 通过。
- 公共目录没有应用名称或应用策略。
- `REQUIRES` 没有引入未使用的 BSP 能力。
- app-specific 行为和资源全部位于 `apps/example`。
- 真机结果记录了板卡版本、串口日志、已验证功能和未验证项。
- release 标签使用 `<app>/v<version>`，不会覆盖其他应用发布。
