# 文档索引

`docs/` 只保存影响多个 workspace member 的文档。应用功能、操作说明和专属
资源文档放在 `apps/<app>`；公共 Rust crate 和 C BSP 的 interface 说明放在
各自目录，避免跨层复制后产生不一致。

## 架构与硬件

| 文档 | 内容 |
| --- | --- |
| [architecture/overview.md](architecture/overview.md) | workspace 模块、稳定 seam、运行任务、Flash 和资源布局 |
| [architecture/hardware.md](architecture/hardware.md) | 板级器件、引脚、共享总线和实机验收约束 |

## 开发与发布

| 文档 | 内容 |
| --- | --- |
| [development/building.md](development/building.md) | 环境、主机测试、独立构建、烧录、打包和发布 |
| [development/adding-apps.md](development/adding-apps.md) | 新应用目录契约、Rust 入口、依赖、测试和独立发布 |
| [../CONTRIBUTING.md](../CONTRIBUTING.md) | 代码规范、自动化检查、真机验证和提交要求 |

## 应用与公共模块

| 入口 | 内容 |
| --- | --- |
| [../apps/README.md](../apps/README.md) | 已发现应用及应用目录职责 |
| [../apps/niulai/README.md](../apps/niulai/README.md) | 牛来功能、开发入口和使用说明 |
| [../apps/diagnostics/README.md](../apps/diagnostics/README.md) | 硬件诊断工具 |
| [../crates/README.md](../crates/README.md) | 公共 Rust crates 与提取规则 |
| [../components/README.md](../components/README.md) | 公共 C BSP 与 Rust 启动入口 |

## 运维记录

| 文档 | 内容 |
| --- | --- |
| [operations/incidents/2026-08-25-lvgl-cross-task-freeze.md](operations/incidents/2026-08-25-lvgl-cross-task-freeze.md) | LVGL 跨任务死机复盘与防复发规则 |
| [../CHANGELOG.md](../CHANGELOG.md) | workspace、公共模块、构建工具和 CI 变更 |

仓库总览见[中文 README](../README.zh_CN.md)。文档移动后必须同步更新引用，并
运行本地 Markdown 链接检查或等价验证。
