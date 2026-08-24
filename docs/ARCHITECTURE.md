# 架构说明

## 设计目标

项目把业务状态、持久存储和板级硬件放在三个清晰接口之后，使可测试逻辑不依赖 ESP-IDF，同时避免为只有一个实现的功能增加额外抽象。

```text
根 CMake PASSPORT_APP 选择器
  ├─ apps/niulai/firmware/app_main
  │   └─ niulai_app_start
  │       ├─ apps/niulai        Rust 牛来状态、动作决策与专属 C ABI
  │       ├─ passport-core      Rust 共用 PCM、电池计算与 C interface
  │       ├─ niulai_voice_store 双 Bank 录音持久化
  │       └─ bsp_* modules      显示、LVGL、按键、音频、电池、I2C
  └─ apps/diagnostics/firmware/app_main
      ├─ passport-core          Rust 共用电池计算与 C interface
      ├─ bsp_i2c
      └─ bsp_battery
```

## 模块

| 模块 | 接口 | 职责 |
| --- | --- | --- |
| `apps/niulai` | `Model::apply` 和现有 C 头文件 | 牛来页面、返回页、音量、动作映射和迁移期 C ABI；`no_std` 且可在主机测试 |
| `apps/diagnostics` | `diagnostics_voltage_percent` 和串口日志 | 最小 I2C 扫描、电量计与共享 Rust core 验证 |
| `passport-core` | Rust 纯函数和 `passport_core.h` | 多工具可共用的 PCM 音量与电池估算；`no_std` 且可在主机测试 |
| `niulai_app` | `niulai_app_start` | LVGL 页面、动画、任务、按键编排和故障降级 |
| `niulai_voice_store` | `init/read/begin/append/finish/reset` | 录音流式写入、校验和双 Bank 原子切换 |
| `components/bsp_*` | 各自的 `bsp_*` 头文件 | 按能力隐藏 GPIO、I2C、I2S、SPI、ADC 和器件初始化细节 |

硬件常量只在 `components/bsp_board/include/bsp_pins.h` 定义。应用不得复制 GPIO、总线地址或屏幕参数。`bsp_audio` 和 `bsp_battery` 依赖 `bsp_i2c`，`bsp_lvgl` 依赖 `bsp_display`；根 CMake 只构建所选 firmware module 的传递依赖。

根 CMake 工程只负责根据 `PASSPORT_APP` 把对应 `apps/<app>/firmware`
注册为 ESP-IDF module。每个应用拥有自己的 Rust 静态库和 C ABI；应用
之间只通过 `passport-core` 源码与 BSP interface 共享行为，不共享应用 adapter。
公共 `sdkconfig.defaults` 只保存板级默认值，每个应用的配置和分区策略位于
`apps/<app>/sdkconfig.defaults` 及应用 firmware 目录；生成的 sdkconfig 位于
`build/<app>/sdkconfig`，切换工具不会污染其他构建。

## 复用决策

diagnostics 只需要 I2C、电池和 `passport-core`，不需要音频、LVGL、按键或
录音存储。因此当前没有第二个音频任务或双 Bank 存储 adapter，相关
implementation 继续留在 `apps/niulai`。当第二个应用出现相同需求时，再以
实际调用方式设计 interface，避免提前引入回调和配置参数组成的浅 module。

## 运行任务

- LVGL 任务负责绘制和动画 timer。
- button 组件通过 ESP-IDF `esp_timer` 任务轮询 ADC 并同步调用应用回调；
  回调必须短小，不能执行音频、Flash 或其他长耗时操作。
- `niulai_audio` 任务串行执行播放、录音、停止和重置命令。
- `niulai_battery` 任务每秒刷新读数，I2C 故障时继续重试。

### LVGL 并发约束

LVGL 不是线程安全的。以下规则是运行时不变量，不因应用数量增加而改变：

1. `Ui`/LVGL 对象只能在 LVGL task 中访问，或在持有
   `display::lock()`（C 侧为 `bsp_lvgl_lock()`）期间访问。
2. button、audio、battery 和其他 `esp_timer`/FreeRTOS 回调不得在取得
   display lock 前调用任何 UI 方法，包括看似简单的图片、文本、可见性和样式更新。
3. 应用状态方法只更新状态，不得隐藏 UI 副作用。由持有 display lock 的
   render 阶段把状态投影到 UI。
4. 同时需要 display lock 和应用状态 mutex 时，固定先取得 display lock，
   再取得状态 mutex；不得持有状态 mutex 再等待 display lock。
5. button/`esp_timer` 回调优先只做有界状态更新或投递命令。现有同步 render
   必须先释放状态 mutex，再取得 display lock；需要等待、播放、录音、存储
   或复杂渲染时，转交专用任务处理。

违反第 1、2 条可能在 `lv_inv_area()` 检测到 `rendering_in_progress` 时进入
LVGL 默认断言死循环，随后表现为 `esp_timer` task watchdog。完整案例见
[2026-08-25 LVGL 跨任务访问死机复盘](incidents/2026-08-25-lvgl-cross-task-freeze.md)。

音频读写只在音频任务执行。

## Niu Lai Flash 布局

| 分区 | 偏移 | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `nvs` | `0x9000` | 24 KB | ESP-IDF NVS |
| `phy_init` | `0xF000` | 4 KB | PHY 初始化数据 |
| `factory` | `0x10000` | 2 MB | 应用与内嵌媒体 |
| `recordings` | `0x210000` | 2 MB | 两个角色的双 Bank PCM 录音 |

每个录音槽使用两个 Bank。写入过程先擦除备用 Bank、流式追加 PCM，再写入包含 magic、版本、采样率、长度和序号的 header。启动时选择序号最新且 header 有效的 Bank；未完成的写入不会替换旧录音。设置页重置会擦除整个 `recordings` 分区，而不是只让录音在 UI 中不可见。

## 素材管线

- `apps/niulai/assets/` 保存可人工查看/试听的 PNG、GIF、WAV 和来源说明。
- `apps/niulai/firmware/assets/` 保存直接嵌入固件的 RGB565 与无 WAV 头 PCM。
- `apps/niulai/firmware/CMakeLists.txt` 是固件素材清单；没有在其中列出的二进制不会进入镜像。

新增媒体前要确认再分发权，并检查 factory 分区余量和 ESP32-C3 内部 RAM。设备没有 PSRAM，音频使用固定大小块流式处理，禁止一次性加载完整录音。

## 故障策略

显示和 LVGL 是硬依赖，初始化失败时应用无法启动。音频、电量计和录音分区是软依赖：对应功能显示不可用，但页面和其他输入应继续工作。

## 测试接口

`scripts/test.sh` 运行 workspace Rust 测试，再自动调用每个应用自己的主机
测试。Niu Lai 的 C 行为测试直接链接 `niulai-app` 静态库，从应用 interface
验证迁移前后的 ABI 行为。CI 自动发现所有含 Rust crate、firmware module
和测试入口的应用，并分别验证交叉编译、驱动、分区、素材符号和依赖集成；
显示、声音、ADC 和电池仍必须上板验收。
