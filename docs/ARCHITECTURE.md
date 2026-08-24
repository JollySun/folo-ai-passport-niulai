# 架构说明

## 设计目标

项目把业务状态、持久存储和板级硬件放在三个清晰接口之后，使可测试逻辑不依赖 ESP-IDF，同时避免为只有一个实现的功能增加额外抽象。

```text
app_main
  └─ niulai_app_start
      ├─ niulai_model          纯状态与动作决策
      ├─ niulai_voice_store    双 Bank 录音持久化
      └─ components/bsp        显示、按键、音频、电池、共享 I2C
```

## 模块

| 模块 | 接口 | 职责 |
| --- | --- | --- |
| `niulai_model` | `init`、`apply` | 页面、返回页、音量和输入到动作的映射；可在主机测试 |
| `niulai_app` | `niulai_app_start` | LVGL 页面、动画、任务、按键编排和故障降级 |
| `niulai_voice_store` | `init/read/begin/append/finish/reset` | 录音流式写入、校验和双 Bank 原子切换 |
| `components/bsp` | `bsp_*` 头文件 | 隐藏 GPIO、I2C、I2S、SPI、ADC 和器件初始化细节 |

硬件常量只在 `components/bsp/include/bsp_pins.h` 定义。应用不得复制 GPIO、总线地址或屏幕参数。

## 运行任务

- LVGL 任务负责绘制和动画 timer。
- button 组件任务只派发轻量事件；回调不能执行阻塞式音频或 Flash 操作。
- `niulai_audio` 任务串行执行播放、录音、停止和重置命令。
- `niulai_battery` 任务每秒刷新读数，I2C 故障时继续重试。

LVGL 不是线程安全的。button、audio 和 battery 上下文修改对象时必须持有 `bsp_lvgl_lock()`，音频读写只在音频任务执行。

## Flash 布局

| 分区 | 偏移 | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `nvs` | `0x9000` | 24 KB | ESP-IDF NVS |
| `phy_init` | `0xF000` | 4 KB | PHY 初始化数据 |
| `factory` | `0x10000` | 2 MB | 应用与内嵌媒体 |
| `recordings` | `0x210000` | 2 MB | 两个角色的双 Bank PCM 录音 |

每个录音槽使用两个 Bank。写入过程先擦除备用 Bank、流式追加 PCM，再写入包含 magic、版本、采样率、长度和序号的 header。启动时选择序号最新且 header 有效的 Bank；未完成的写入不会替换旧录音。设置页重置会擦除整个 `recordings` 分区，而不是只让录音在 UI 中不可见。

## 素材管线

- `assets/niulai/` 保存可人工查看/试听的 PNG、GIF、WAV 和来源说明。
- `main/assets/niulai/` 保存直接嵌入固件的 RGB565 与无 WAV 头 PCM。
- `main/CMakeLists.txt` 是固件素材清单；没有在其中列出的二进制不会进入镜像。

新增媒体前要确认再分发权，并检查 factory 分区余量和 ESP32-C3 内部 RAM。设备没有 PSRAM，音频使用固定大小块流式处理，禁止一次性加载完整录音。

## 故障策略

显示和 LVGL 是硬依赖，初始化失败时应用无法启动。音频、电量计和录音分区是软依赖：对应功能显示不可用，但页面和其他输入应继续工作。

## 测试接口

`scripts/test.sh` 只通过模块公开接口验证纯逻辑，不依赖内部静态状态。完整 ESP-IDF 构建验证驱动、分区、素材符号和依赖集成；显示、声音、ADC 和电池仍必须上板验收。
