# FoloToy AI Passport 小工具集合

[English](README.md) | 简体中文

[![CI](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml/badge.svg)](https://github.com/JollySun/folo-ai-passport-niulai/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/code%20license-MIT-green.svg)](LICENSE)

这是面向 ESP32-C3 FoloToy AI Passport 的 monorepo，用于开发、测试和独立发布
多个小工具。应用行为使用 Rust；公共 C 代码只提供与应用无关的板级和 ESP-IDF
机制，并通过 Rust facade 暴露给应用。

## 应用

| 应用 | 用途 | 构建命令 | 发布标签 |
| --- | --- | --- | --- |
| [牛来](apps/niulai/README.md) | 离线动画、对白播放、录音和电量界面 | `scripts/build-app.sh niulai build` | `niulai/v<version>` |
| [硬件诊断](apps/diagnostics/README.md) | 最小 I2C 和电量计诊断 | `scripts/build-app.sh diagnostics build` | `diagnostics/v<version>` |

每个应用在 `apps/<app>` 下拥有自己的 Rust crate、firmware 元数据、测试、配置、
分区策略和资源。公共 workspace 不维护应用注册表或应用专属 switch。

## Workspace 结构

```text
apps/                      可独立构建和发布的工具及其资源
crates/                    可复用 Rust 模块
  passport-core/           硬件无关的 no_std 计算
  passport-platform/       BSP 与 ESP-IDF 能力的 Rust facade
components/                通用 C 板级机制和稳定 Rust 入口
cmake/                     共用 Rust 静态库构建集成
scripts/                   应用发现、测试、构建和打包入口
docs/                      跨应用架构和开发文档
```

稳定的 C `app_main` 只调用所选 Rust 应用导出的 `passport_app_main`。只要
`apps/<app>` 满足目录契约，新增应用无需修改根 Cargo workspace、根 CMake、
CI matrix 或 release workflow。完整依赖和运行模型见
[Monorepo 架构](docs/architecture/overview.md)。

## 快速开始

使用 ESP-IDF 5.5.3 和 `rust-toolchain.toml` 固定的 Rust 工具链：

```bash
scripts/list-apps.sh
scripts/test.sh
scripts/build-app.sh niulai build
scripts/build-app.sh niulai flash monitor
```

每个应用的构建输出隔离在 `build/<app>`。`<app>/v*` 标签只构建和发布对应应用。

## 文档

- [文档索引](docs/README.md)
- [Monorepo 架构](docs/architecture/overview.md)
- [构建、烧录与发布](docs/development/building.md)
- [新增独立应用](docs/development/adding-apps.md)
- [板级硬件](docs/architecture/hardware.md)
- [贡献指南](CONTRIBUTING.md)
- [Workspace 变更记录](CHANGELOG.md)

应用使用说明和资源许可与对应应用放在一起，不进入跨应用文档目录。

## 许可证

源代码使用 [MIT License](LICENSE)。应用媒体可能采用不同的再分发条款，发布
固件或素材前必须查看对应应用的资源说明。
