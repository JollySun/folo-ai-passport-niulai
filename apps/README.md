# Applications

`apps/<app>` 是 monorepo 中可独立构建、测试、烧录和发布的 workspace member。
应用之间不能互相依赖，也不能要求公共代码维护应用名或注册表。

| 应用 | 说明 | 专属能力 |
| --- | --- | --- |
| [niulai](niulai/README.md) | 牛来离线互动播放器 | LVGL、按键、音频、录音分区、电池 |
| [diagnostics](diagnostics/README.md) | 最小硬件诊断工具 | runtime、I2C、电池 |

每个应用目录必须包含 `Cargo.toml`、`firmware/CMakeLists.txt`、
`sdkconfig.defaults` 和可执行的 `test.sh`。应用专属状态、任务、UI、持久化
策略、分区、测试和资源都属于该目录。

应用版本历史记录在 `apps/<app>/CHANGELOG.md`；根 `CHANGELOG.md` 只记录
workspace、公共模块、构建工具和 CI 变化。

新增应用的完整模板见[开发指南](../docs/development/adding-apps.md)。实际发现结果：

```bash
scripts/list-apps.sh
scripts/list-apps.sh --json
```
