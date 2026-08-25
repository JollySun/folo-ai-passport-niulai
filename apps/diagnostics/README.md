# 硬件诊断工具

这是一个可独立构建和发布的最小 Rust 应用，用于扫描共享 I2C 总线、探测
CW2017 电量计，并输出电压、芯片 SOC 和公共 Rust 电压估算结果。

它只依赖 runtime、I2C、电池和 `passport-core`，不会引入显示、LVGL、按键、
音频或存储模块。因此它同时验证 monorepo 能否按应用裁剪公共 BSP 能力。

```bash
scripts/test.sh
scripts/build-app.sh diagnostics build
scripts/build-app.sh diagnostics flash monitor
```

串口日志是该工具的用户界面。电量计缺失时应用会记录降级信息，而不是让固件
启动失败。

推送 `diagnostics/v<version>` 标签可独立发布，不会构建或发布其他应用。
发布前更新[诊断工具变更记录](CHANGELOG.md)。
