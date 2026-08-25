# Monorepo 架构

## 设计目标

项目支持在同一块 ESP32-C3 板卡上开发、测试和独立发布多个小工具。
应用行为使用 Rust 实现；与具体应用无关的板级和 ESP-IDF 基础能力可以使用 C，
但必须通过稳定的 Rust interface 提供给应用。新增应用不应修改公共入口、
应用注册表或其他应用代码。

```text
scripts/list-apps.sh 自动发现 apps/*
  -> 根 CMake 用 PASSPORT_APP 选择 apps/<app>/firmware
     -> firmware component 声明该应用实际需要的 BSP 能力
     -> cmake/passport_rust_app.cmake 构建并链接应用 Rust staticlib
     -> components/rust_app_entry/app_main
        -> 稳定符号 passport_app_main
           -> apps/<app>                 Rust 应用行为和专属资源
              ├─ crates/passport-core    纯 Rust、硬件无关算法
              └─ crates/passport-platform
                 -> components/bsp_*     通用 C/ESP-IDF 基础能力
```

## 模块

| 模块 | 接口 | 职责 |
| --- | --- | --- |
| `apps/<app>` | Rust crate；固件统一导出 `passport_app_main` | 一个工具的全部状态、流程、任务、UI 编排、测试和专属资源 |
| `apps/<app>/firmware` | ESP-IDF component | 只声明依赖、应用配置、分区和嵌入资源，不承载手写应用行为 |
| `crates/passport-core` | `no_std` Rust 纯函数 | 无硬件依赖、可供多个工具复用的 PCM 和电池计算 |
| `crates/passport-platform` | Rust modules 和 RAII guards | 面向应用的通用 Rust 硬件/runtime/UI facade，集中管理 C FFI |
| `components/bsp_*` | 各自的 `bsp_*` 头文件 | 按能力隐藏 GPIO、I2C、I2S、SPI、ADC 和器件初始化细节 |
| `components/rust_app_entry` | `app_main` → `passport_app_main` | 稳定且与应用无关的唯一 C 启动入口 |
| `cmake/passport_rust_app.cmake` | `passport_add_rust_staticlib` | 交叉编译所选 Rust crate，并保证 static archive 的链接顺序 |

硬件常量只在 `components/bsp_board/include/bsp_pins.h` 定义。应用不得复制
GPIO、总线地址或屏幕参数。`bsp_audio` 和 `bsp_battery` 依赖 `bsp_i2c`，
`bsp_lvgl` 依赖 `bsp_display`；根 CMake 只构建所选 firmware component 的
传递依赖，未声明的显示、音频等能力不会进入该工具的固件。

根 CMake 工程只负责根据 `PASSPORT_APP` 把对应 `apps/<app>/firmware`
注册为 ESP-IDF component。每个应用拥有自己的 Rust 静态库，并实现同一个
`passport_app_main` 符号；公共代码不维护应用名、switch 或注册表。应用之间
只通过 `passport-core` 和 `passport-platform` 共享能力，不互相依赖。
公共 `sdkconfig.defaults` 只保存板级默认值，每个应用的配置和分区策略位于
`apps/<app>/sdkconfig.defaults` 及应用 firmware 目录；生成的 sdkconfig 位于
`build/<app>/sdkconfig`，切换工具不会污染其他构建。

### 稳定边界

- `crates/`、`components/`、`cmake/`、根 CMake 和通用 scripts 中不得出现
  应用名称；`scripts/check-architecture.sh` 在主机测试中验证这一约束。
- `components/rust_app_entry` 永远只调用 `passport_app_main`。新增应用不修改它。
- 应用目录中不允许新增手写 C 行为代码；firmware 目录只保留 CMake、分区、
  二进制资源及带生成来源标记的字体等生成文件。
- C BSP 只提供板级机制，Rust `passport-platform` 提供面向应用的类型和资源
  生命周期。应用专属策略不得下沉到这两层。
- 只有出现第二个真实调用方并确认语义一致后，应用代码才提取到公共 crate；
  不为假设中的未来需求提前增加 adapter 或配置层。

## 复用决策

`diagnostics` 只需要 runtime、I2C、电池和 `passport-core`，不需要显示、
LVGL、按键、音频或存储。Niu Lai 的状态机、UI、媒体、任务编排和双 Bank
录音协议继续位于 `apps/niulai`；其中只有原始分区读写使用通用
`passport-platform::storage`/`bsp_storage`。这说明复用边界按能力选择，
而不是把一个完整应用包装成另一个应用的公共依赖。

新增工具的目录契约、最小模板和独立发布步骤见[新增应用指南](../development/adding-apps.md)。

## 运行时约束

`rust_app_entry` 只转交启动控制，应用自行创建所需任务和 queue。公共 platform
不会隐藏应用任务、重试策略或调度周期。button 组件通过 ESP-IDF `esp_timer`
任务轮询 ADC 并同步调用 Rust callback；进入 Rust 不会改变当前任务上下文，
因此 callback 必须短小，不能执行音频、Flash 或其他长耗时操作。

LVGL 任务负责绘制及 LVGL timer。音频、存储和其他可能阻塞的工作由应用投递
到专用任务，不能占用 button callback 或 LVGL task。

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
[2026-08-25 LVGL 跨任务访问死机复盘](../operations/incidents/2026-08-25-lvgl-cross-task-freeze.md)。

## 应用专属设计

任务名、页面、分区格式、素材管线和故障降级策略属于具体应用，不在公共架构
文档复制：

- [牛来应用](../../apps/niulai/README.md)：运行任务、录音分区、素材和软依赖策略。
- [硬件诊断](../../apps/diagnostics/README.md)：最小依赖集和串口输出行为。

## 测试接口

`scripts/test.sh` 检查 Rust 格式、运行 workspace Rust 测试、验证架构边界和
公共板卡头文件，再自动调用每个已发现应用的 `test.sh`。CI 使用相同的应用
发现结果建立 matrix，为每个工具独立完成 ESP-IDF 交叉编译和固件打包。

主机测试验证纯逻辑和边界契约；交叉编译验证 Rust/C 链接、依赖、分区和资源
符号。显示、按键、声音、ADC、电池、并发及时序结论仍必须上板验收。

## 文档归属

- 根 README 只说明 workspace、应用清单和统一入口。
- `docs/` 保存跨应用架构、构建、硬件与运维规则。
- `apps/<app>/README.md` 和 `apps/<app>/docs/` 保存应用行为与使用说明。
- `crates/<crate>/README.md` 描述公共 Rust interface 和约束。
- `components/README.md` 描述公共 C 模块及依赖关系。

内容应链接到所属位置，不在多个层级复制同一份参数、流程或行为说明。
