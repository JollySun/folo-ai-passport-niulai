# 硬件说明

本文只记录当前代码和实机已经使用的能力，不把 ESP32-C3 数据手册中的潜在能力当作本板能力。硬件事实冲突时，优先级为：原理图/PCB/实测 > `bsp_pins.h` > BSP 实现 > 本文。

## 已实现硬件

| 子系统 | 器件/方式 | 当前实现 |
| --- | --- | --- |
| MCU | ESP32-C3、8 MB Flash、无 PSRAM | ESP-IDF 5.5.3 |
| 显示 | ST7789P3、240×320、RGB565 | SPI2 40 MHz，LEDC 背光 |
| 输入 | UP/DOWN/OK 电阻分压 | GPIO0 / ADC1_CH0 |
| 音频 | ES8311 | I2S0 全双工，I2C 控制，播放与麦克风录音 |
| 电池 | CW2017 | I2C SOC/电压，可缺省并降级 |
| 日志 | USB Serial/JTAG | ESP32-C3 原生 USB |

## 引脚

| GPIO | 功能 |
| ---: | --- |
| 0 | 三键公共 ADC 节点（ADC1_CH0） |
| 1 | LCD CS |
| 2 | I2S DOUT（MCU → codec） |
| 3 | I2S WS |
| 4 | I2S DIN（codec → MCU） |
| 5 | I2S BCLK |
| 6 | I2S MCLK |
| 7 | I2C SCL |
| 8 | LCD SCLK |
| 9 | LCD MOSI |
| 10 | I2C SDA |
| 18/19 | USB Serial/JTAG |
| 20 | LCD DC |
| 21 | LCD 背光 PWM |

LCD RST 和功放使能在当前板级配置中为 `-1`。未列出的 GPIO 不等于可安全使用，必须先核对原理图、封装和 Flash 连接。

## 关键约束

- ES8311（7 bit 地址 `0x18`）和 CW2017（`0x63`）共享 I2C0；不能创建第二条 I2C0 总线。
- 三个按键共享一个 ADC1 unit；不能再创建独立 ADC1 oneshot unit。
- 当前按键窗口为 UP `[0,150)` mV、DOWN `[150,447)` mV、OK `[447,1900)` mV。更换电阻后必须实测再修改 `BSP_BTN_MV_TABLE`。
- GPIO21 同时是常见 UART0 默认 TX，本项目使用 USB Serial/JTAG，避免串口和背光冲突。
- LCD 为 MOSI-only，无 MISO、触摸或已知 TE 接口；不要宣称支持屏幕读回。
- ESP32-C3 无 PSRAM。LVGL、LCD DMA、I2S DMA、任务栈和音频块都占内部 RAM。
- `bsp_audio_set_format()` 在格式变化时必须保留 close/open 流程，否则采样率可能未真正更新。
- CW2017 读数是电池估算，不是量产校准结果；芯片缺失时应用应继续运行。

## 修改驱动后的验收

| 修改类型 | 必须实测 |
| --- | --- |
| 引脚/I2C | 0x18/0x63 应答、USB 日志、共享设备同时工作 |
| LCD | 方向、裁切、RGB/字节序、反色、背光等级 |
| ADC/按键 | 松开与三键电压、短按/双击/长按、供电变化裕量 |
| codec/I2S | 12 kHz 默认声音、16 kHz 自定义录音、播放速度/音调、非零录音、格式切换、并发刷屏 |
| 电池 | SOC/mV 合理、芯片缺失和间歇 I2C 故障降级 |
| 内存/DMA | 构建内存报告、运行最大连续堆、长时间重复操作 |

仓库尚未包含原理图、PCB/BOM、板卡修订号、电池/充电参数、LCD 完整料号和功放信息。因此低功耗、充电控制、未用引脚复用或音频功率修改必须先取得硬件资料。
