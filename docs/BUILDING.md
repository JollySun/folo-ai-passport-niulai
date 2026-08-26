# 构建说明

## 环境要求

- ESP-IDF 5.5.3（CI 使用 `espressif/idf:v5.5.3`）
- Git、C 编译器和支持数据传输的 USB 线
- ESP32-C3 FoloToy AI Passport，8 MB Flash

按 [ESP-IDF 官方安装说明](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32c3/get-started/index.html) 安装工具链。项目依赖版本记录在已提交的 `dependencies.lock` 中，不要编辑 `managed_components/`。

## 首次构建

```bash
git clone https://github.com/JollySun/folo-ai-passport-niulai.git
cd folo-ai-passport-niulai
source "$HOME/esp/esp-idf/export.sh"
idf.py set-target esp32c3
scripts/test.sh
idf.py build
```

`sdkconfig.defaults` 已配置 ESP32-C3、8 MB Flash、USB Serial/JTAG、LVGL 和自定义分区表。`sdkconfig` 是本机构建产物，不应提交。

## 主机测试

```bash
scripts/test.sh
```

该命令在临时目录编译并运行应用状态机、PCM 音量、电池电压换算、UI 状态切换和 12 px 字体字形覆盖测试，不需要 ESP-IDF 或实体设备。可使用 `CC=clang scripts/test.sh` 切换编译器。

## 烧录与日志

开发时直接构建、烧录并打开日志：

```bash
idf.py build
idf.py flash monitor
```

退出 monitor 使用 `Ctrl+]`。如果设备端口未被自动识别，添加 `-p /dev/your-port`。

仅更新应用分区：

```bash
idf.py app-flash monitor
```

仅当设备已经包含本项目的分区表时才使用 `app-flash`。首次安装必须写入整机镜像，否则录音存储不可用。

## 生成发布文件

先完成构建，再运行：

```bash
scripts/package-firmware.sh build dist dev
```

`dist/` 将包含：

- `*-full.bin`：地址 `0x0`，包含 bootloader、分区表和应用；
- `*-app.bin`：地址 `0x10000`，仅应用；
- `*-bootloader.bin` 和 `*-partition-table.bin`：诊断/高级烧录；
- `SHA256SUMS`：下载校验。

使用 esptool 烧录整机镜像：

```bash
esptool.py --chip esp32c3 write_flash 0x0 dist/folo-ai-passport-niulai-dev-full.bin
```

`build/`、`dist/`、`managed_components/` 和 `sdkconfig` 都是本地生成内容，已由 Git 忽略。

## CI 与发布

- `.github/workflows/ci.yml` 在拉取请求、`main` 推送和手动触发时执行主机测试、空白检查、完整固件构建和打包。
- `.github/workflows/release.yml` 在推送 `v*` 标签时重复构建，并创建带校验和的 GitHub Release。
- Actions 依赖由 Dependabot 每月检查。

创建发布前先更新 `CHANGELOG.md`，确认工作区干净并完成实机验收，然后推送带注释标签：

```bash
git tag -a v1.0.0 -m "v1.0.0"
git push origin v1.0.0
```

## 实机验收

自动构建不能替代硬件验证。至少检查：稳定启动、显示方向和颜色、三键短按/长按、上/下键连续按压、首页双击确认键无动作、两段 12 kHz 默认声音、12/16 kHz 格式切换、音量 0/50/100%、两路录音和重启后持久化、自定义声音结束时动画同步停止、设置页提示无缺字、电量降级，以及恢复默认声音。
