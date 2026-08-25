# 牛来互动播放器

牛来是 FoloToy AI Passport 上的离线互动应用，提供角色动画、对白播放、音量
设置、电量显示和用户录音替换。应用行为、UI、录音策略、测试及全部专属资源
均位于本目录。

<p align="center">
  <img src="assets/home-ui-preview.png" alt="牛来首页预览" width="300">
</p>

> [!IMPORTANT]
> 这是非官方爱好者原型。仓库 MIT 许可证只覆盖源代码，不自动授权电影图片和
> 音频。重新分发固件或素材前请阅读[素材来源与分发提示](assets/SOURCES.md)。

## 功能和使用

- 牛来和妈妈双帧动画及 16 kHz 单声道对白。
- 0–100% 软件播放音量。
- 每个角色最多 10 秒的自定义录音。
- 独立 Flash 分区和双 Bank 原子切换，保存失败时保留上一份有效录音。
- CW2017 电量显示及电压估算降级。
- 三个实体按键操作，无需网络或账号。

安装、按键、录音、设置和故障排查见[使用说明](docs/user-guide.md)。

## 目录职责

```text
src/                 Rust 状态模型、任务、UI 编排和录音协议
firmware/            CMake、分区、嵌入资源和生成字体
assets/              可审查的源素材、预览及许可说明
sdkconfig.defaults   牛来专属 ESP-IDF 配置
test.sh              牛来主机测试入口
```

`firmware/` 中的 C 文件仅为带来源标记的生成字体；不在 C 中实现应用行为。
原始分区访问复用 `passport-platform::storage`，双 Bank 录音格式和提交策略仍属于
牛来应用。

## 运行任务与降级

- LVGL task 负责绘制和 120 ms 动画 timer。
- button callback 只更新状态和投递命令，释放状态锁后通过统一 render 更新 UI。
- `niulai_audio` 串行执行播放、录音、停止和恢复默认声音。
- `niulai_battery` 每秒刷新电量，I2C 故障时继续重试。

显示和 LVGL 是硬依赖，初始化失败时应用无法启动。音频、电量计和录音分区是
软依赖：对应功能显示不可用，但页面和其他输入继续工作。

## Flash 和录音格式

| 分区 | 偏移 | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `nvs` | `0x9000` | 24 KB | ESP-IDF NVS |
| `phy_init` | `0xF000` | 4 KB | PHY 初始化数据 |
| `factory` | `0x10000` | 2 MB | 应用与内嵌媒体 |
| `recordings` | `0x210000` | 2 MB | 两个角色的双 Bank PCM 录音 |

每个录音槽使用两个 Bank。保存时先擦除备用 Bank、流式追加 PCM，再写入包含
magic、版本、采样率、长度和序号的 header。启动时选择序号最新且 header 有效
的 Bank；未完成写入不会替换旧录音。恢复默认声音会擦除整个 `recordings` 分区。

## 素材管线

- `assets/` 保存可人工查看或试听的 PNG、GIF、WAV、预览和来源说明。
- `firmware/assets/` 保存直接嵌入固件的 RGB565 与无 WAV 头 PCM。
- `firmware/CMakeLists.txt` 是固件素材清单；未列出的二进制不会进入镜像。

新增媒体前必须确认再分发权，并检查 factory 分区余量和内部 RAM。ESP32-C3
没有 PSRAM，音频使用固定大小块流式处理，不能一次性加载完整录音。

## 开发

从仓库根目录运行：

```bash
scripts/test.sh
scripts/build-app.sh niulai build
scripts/build-app.sh niulai flash monitor
scripts/package-firmware.sh niulai build/niulai dist/niulai dev
```

涉及 UI、callback、task、mutex 或 FFI 的修改必须遵守
[LVGL 并发约束](../../docs/architecture/overview.md#lvgl-并发约束)，并完成贡献
[指南](../../CONTRIBUTING.md#local-verification)要求的真机串口监控。

推送 `niulai/v<version>` 标签会独立构建并发布牛来固件，不影响其他应用。
发布前更新[牛来变更记录](CHANGELOG.md)。
