# 构建说明

## 环境要求

- ESP-IDF 5.5.3（CI 使用 `espressif/idf:v5.5.3`）
- Rustup；仓库的 `rust-toolchain.toml` 会安装固定 nightly、`rust-src`、
  `rustfmt` 和 `clippy`
- Git、C 编译器和支持数据传输的 USB 线
- ESP32-C3 FoloToy AI Passport，8 MB Flash

按 [ESP-IDF 官方安装说明](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32c3/get-started/index.html)
安装 ESP-IDF，并按 [Rustup](https://rustup.rs/) 说明安装 Rustup。项目的
Rust 版本和组件由 `rust-toolchain.toml` 固定，Cargo 依赖由 `Cargo.lock`
固定；ESP-IDF 依赖记录在 `dependencies.lock` 中。不要编辑
`managed_components/`。

## 首次构建

```bash
git clone https://github.com/JollySun/folo-ai-passport-niulai.git
cd folo-ai-passport-niulai
source "$HOME/esp/esp-idf/export.sh"
scripts/build-app.sh niulai set-target esp32c3
scripts/test.sh
scripts/build-app.sh niulai build
```

根 `sdkconfig.defaults` 配置 ESP32-C3、8 MB Flash 和 USB Serial/JTAG；
`apps/<app>/sdkconfig.defaults` 保存应用自己的 UI 与分区策略。生成配置写入
`build/<app>/sdkconfig`，不会在切换工具时共用。

`scripts/build-app.sh` 把应用名传给根 CMake 的 `PASSPORT_APP`，后者只注册
`apps/<app>/firmware` 及其声明的传递依赖。应用 Rust crate 由
`cmake/passport_rust_app.cmake` 交叉编译为 static library，并通过通用
`components/rust_app_entry` 的 `passport_app_main` 符号启动。

## 主机测试

```bash
scripts/test.sh
```

该命令检查 Rust 格式、运行 workspace Rust 单元测试、执行
`scripts/check-architecture.sh`、验证公共板卡头文件，再自动调用每个已发现
应用的 `test.sh`。它覆盖应用状态机、PCM 音量、电池电压换算和公共目录边界，
不需要 ESP-IDF 或实体设备。可使用 `CC=clang scripts/test.sh` 切换 C 编译器。

以下四个文件同时存在且 `test.sh` 可执行时，应用才会被本地脚本和 CI 自动发现：

```text
apps/<app>/Cargo.toml
apps/<app>/firmware/CMakeLists.txt
apps/<app>/sdkconfig.defaults
apps/<app>/test.sh
```

`apps/` 下存在命名不合法或不满足该契约的目录时，发现步骤会明确失败，不会把它
静默排除在 CI firmware matrix 之外。

新增工具无需修改根 workspace、CMake、CI matrix 或发布 workflow。完整模板见
[新增应用指南](ADDING_APPS.md)。

## 烧录与日志

开发时直接构建、烧录并打开日志：

```bash
scripts/build-app.sh niulai build
scripts/build-app.sh niulai flash monitor
```

最小诊断工具不包含显示、LVGL 或音频，可独立构建和烧录：

```bash
scripts/build-app.sh diagnostics build
scripts/build-app.sh diagnostics flash monitor
```

退出 monitor 使用 `Ctrl+]`。如果设备端口未被自动识别，添加 `-p /dev/your-port`。

仅更新应用分区：

```bash
scripts/build-app.sh niulai app-flash monitor
```

仅当设备已经包含本项目的分区表时才使用 `app-flash`。首次安装必须写入整机镜像，否则录音存储不可用。

## 生成发布文件

先完成构建，再运行：

```bash
scripts/package-firmware.sh niulai build/niulai dist/niulai dev
```

`dist/` 将包含：

- `*-full.bin`：地址 `0x0`，包含 bootloader、分区表和应用；
- `*-app.bin`：地址 `0x10000`，仅应用；
- `*-bootloader.bin` 和 `*-partition-table.bin`：诊断/高级烧录；
- `SHA256SUMS`：下载校验。

使用 esptool 烧录整机镜像：

```bash
python -m esptool --chip esp32c3 write-flash 0x0 dist/niulai/folo-ai-passport-niulai-dev-full.bin
```

`build/`、`target/`、`dist/`、`managed_components/` 和 `sdkconfig` 都是本地生成内容，已由 Git 忽略。

调试崩溃或 watchdog 时，应保留与被测 `.bin` 完全一致的
`build/<app>/FoloToy-AI-Passport.elf` 和 `.map`。发布包只保证提供可烧录镜像
和校验和，不能用另一提交的 ELF 解析设备地址。

## CI 与发布

- `.github/workflows/build.yml` 自动发现 `apps/` 下可构建的工具，在拉取请求、推送和手动触发时执行主机测试、空白检查，并分别完成固件构建和打包。
- `.github/workflows/release.yml` 在推送 `<app>/v*` 标签时只构建对应应用，并创建带校验和的独立 GitHub Release。
- Actions 依赖由 Dependabot 每月检查。

创建发布前先更新 `CHANGELOG.md`，确认工作区干净并完成实机验收，然后推送带注释标签：

```bash
git tag -a niulai/v1.0.0 -m "niulai v1.0.0"
git push origin niulai/v1.0.0
```

其他工具替换标签前缀即可，例如 `diagnostics/v1.0.0`。

## 实机验收

自动构建不能替代硬件验证。至少检查：稳定启动、显示方向和颜色、三键短按/长按/双击、两段默认声音、音量 0/50/100%、两路录音和重启后持久化、电量降级，以及恢复默认声音。

涉及 UI、callback、task、mutex 或 FFI 的修改还必须按[贡献指南](../CONTRIBUTING.md#local-verification)
进行至少三分钟串口监控，并确认没有 watchdog、LVGL assertion、panic 或 reboot。
