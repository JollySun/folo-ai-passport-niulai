# 2026-08-25 LVGL 跨任务访问死机复盘

## 结论

Rust 迁移版在 button 组件的 `esp_timer` 回调中、尚未取得 display lock 时
修改了 LVGL 图片对象。若此时 LVGL task 正在渲染，`lv_inv_area()` 会触发
线程安全断言；项目启用的 LVGL 默认断言处理器是 `while(1)`，因此设备不会
立即重启，而是停在死循环并持续触发 task watchdog。

修复提交为 `054158e`：动画启动只更新 Rust 状态，首帧由持有 display lock
的统一 render 路径设置。

## 影响与现象

- 受影响版本：Rust 应用迁移后、`054158e` 之前的固件。
- 用户现象：设备容易无响应，纯 C 版本未复现。
- 最小复现：曾在无人工操作的待机测试中约 88 秒出现；ADC 按键轮询可能产生
  事件，因此“未主动按键”不代表没有进入 button 回调。
- 串口随后每 5 秒重复报告 watchdog，没有自动恢复：

```text
task_wdt: Task watchdog got triggered
task_wdt: - IDLE (CPU 0)
task_wdt: CPU 0: esp_timer
MEPC: 0x420154d2  RA: 0x420154c4
```

## 证据链

1. 多次 watchdog 的 `MEPC`、`RA` 和当前任务完全一致，排除了随机跳转。
2. `0x420154d2` 的指令是跳向自身的压缩 RISC-V `c.j 0`。
3. 使用历史本机构建的 ELF/map 对相同机器码定位后，确认该地址属于
   `lv_inv_area()`，停点对应：

   ```c
   LV_ASSERT_MSG(!disp->rendering_in_progress,
                 "Invalidate area is not allowed during rendering.");
   ```

4. 寄存器中的 display 状态包含 `rendering_in_progress`，与断言条件一致。
5. button 组件通过 `esp_timer` 执行回调；Rust `on_button()` 调用的
   `AppState::start_animation()` 在 display lock 外执行
   `ui.set_animation_frame()`。
6. 纯 C 版本将相同的动画首帧更新放在 `bsp_lvgl_lock()` 保护区内，解释了
   为什么相同硬件和公共 C 组件没有复现。

调用链如下：

```text
esp_timer task
  -> button polling callback
  -> Rust on_button
  -> AppState::start_animation
  -> Ui::set_animation_frame       (当时未持有 display lock)
  -> LVGL invalidation
  -> lv_inv_area assertion
  -> while(1)
  -> task watchdog
```

## 根因

迁移时保留了 C button 驱动，但错误地把 FFI 回调当成普通 Rust 调用处理。
语言边界不会改变执行上下文：进入 Rust 后，代码仍运行在 `esp_timer` task。

同时，`AppState::start_animation()` 名义上是状态方法，内部却隐藏了 UI 写入，
使调用者无法从接口判断它需要 LVGL 锁。这两点共同造成了跨任务访问。

## 修复

从 `AppState::start_animation()` 删除 `ui.set_animation_frame()`。方法现在只更新：

- `animation_active`
- `animation_frame`
- `animation_left_ms`

`on_button()` 释放应用状态 mutex 后调用统一 `render()`；`render()` 先取得
display lock，再读取状态并更新 UI。活动页渲染本来就会设置动画第一帧，
因此不改变用户可见行为。

## 防复发规则

规则的规范版本位于[架构说明](../../architecture/overview.md#lvgl-并发约束)。代码评审至少确认：

- 每个 `Ui`/LVGL 调用点都有明确的执行上下文或 display-lock guard。
- 状态方法没有 UI、音频、Flash 等隐藏副作用。
- FFI callback 的线程/任务上下文已从 C 实现核实，不能依据 Rust 函数签名推断。
- 获取多个锁时遵守 display lock → application-state mutex 的固定顺序。
- button/`esp_timer` 回调保持有界；同步 render 必须先释放状态 mutex 并取得
  display lock，其他复杂工作通过 queue 交给专用任务。

## 验证

修复后完成：

- `scripts/test.sh`：通过。
- ESP-IDF 5.5.3 的 `niulai` 和 `diagnostics` CI 构建：通过。
- ESP32-C3 rev 1.1 全量刷写和镜像校验：通过。
- 串口连续待机超过 3 分钟：无 `task_wdt`、LVGL assertion、panic 或 reboot。

显示、按键、播放、录音和存储属于硬件行为。涉及相关路径的后续修改仍需按
[贡献指南](../../../CONTRIBUTING.md#local-verification)完成交互式真机测试，不能只以
主机测试或固件成功构建作为验收依据。

## 后续诊断方法

若再次出现 watchdog：

1. 保存完整 115200-baud 串口日志，不要只记录最后一条错误。
2. 记录当前 task、`MEPC`、`RA` 和首次发生时间，确认地址是否稳定。
3. 保留与被测固件完全一致的 `.elf` 和 `.map`；优先使用 ELF 符号或 map
   将 PC 映射到函数，不根据任务名直接猜测根因。
4. 调试构建可启用 `CONFIG_ESP_SYSTEM_USE_FRAME_POINTER` 获取完整 backtrace，
   发布构建是否启用需单独评估包体积和性能。
5. 修复后必须重复原触发路径，并观察超过首次故障时间至少两倍。
