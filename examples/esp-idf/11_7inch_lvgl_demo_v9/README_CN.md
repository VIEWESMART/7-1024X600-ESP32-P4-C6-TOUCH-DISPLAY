# ESP32-P4-S070-WBA LVGL 示例（v9）

面向 **ESP32-P4-S070-WBA** 开发板的 LVGL 9 示例，配套 7 寸 **1024×600** MIPI DSI 屏幕。

支持的 LCD 面板（可在 `menuconfig` 中选择）：

| 面板 | 分辨率 |
|------|--------|
| JD9165 | 1024×600 |
| EK79007 | 1024×600 |

## 环境要求

- ESP-IDF **v5.4+**（已在 v5.5.x 验证）
- 芯片目标：**ESP32-P4**
- 开发板：ESP32-P4-S070-WBA（需 PSRAM）

## 使用流程

### 1. 配置 ESP-IDF 环境

可选以下两种方式之一。

#### 方式 A：VS Code + ESP-IDF 插件（推荐）

1. 安装 [Visual Studio Code](https://code.visualstudio.com/)
2. 在扩展市场安装 **Espressif IDF** 插件（发布者：Espressif Systems）
3. 按 `F1`，运行 `ESP-IDF: Configure ESP-IDF Extension`，按向导完成 IDF / 工具链安装或选择已有安装路径
4. 用 VS Code 打开本工程目录 `10_7inch_lvgl_demo_v9`
5. 确认状态栏或命令面板中目标芯片为 **esp32p4**；串口选择实际连接的端口（如 `COM3`）

后续可直接使用 VS Code 状态栏 / 命令面板操作：

| 操作 | 常用入口 |
|------|----------|
| 设置目标 | `ESP-IDF: Set Espressif Device Target` → `esp32p4` |
| menuconfig | `ESP-IDF: SDK Configuration Editor (menuconfig)` |
| 编译 | `ESP-IDF: Build your project` |
| 烧录 | `ESP-IDF: Flash your project` |
| 监视 | `ESP-IDF: Monitor your device` |
| 一键烧录+监视 | `ESP-IDF: Flash and start a monitor on a device` |

> 若插件提示环境未配置，重新执行一次 `ESP-IDF: Configure ESP-IDF Extension`。

#### 方式 B：命令行

```bash
# Linux / macOS
. $HOME/esp/esp-idf/export.sh

# Windows（ESP-IDF PowerShell / CMD）
export.bat
# 或：. .\export.ps1
```

### 2. 设置编译目标

**VS Code：** `F1` → `ESP-IDF: Set Espressif Device Target` → 选择 `esp32p4`

**命令行：**

```bash
cd 10_7inch_lvgl_demo_v9
idf.py set-target esp32p4
```

### 3. 打开 menuconfig

**VS Code：** `F1` → `ESP-IDF: SDK Configuration Editor (menuconfig)`

**命令行：**

```bash
idf.py menuconfig
```

#### 3.1 芯片版本配置（重要）

进入路径：

```text
(Top) → Component config → Hardware Settings → Chip revision
→ Select ESP32-P4 revisions <3.0 (No >=3.x Support)
```

| 你的 ESP32-P4 芯片版本 | 操作 |
|------------------------|------|
| **3.0 以下**（如 0.x / 1.x） | **开启**该选项 |
| **3.0 及以上** | **关闭**该选项 |

> 芯片版本配置错误可能导致无法启动或运行异常。

#### 3.2 选择 LCD 面板

进入路径：

```text
(Top) → Board Support Package (ESP32-P4-S070-WBA) → Display → Select LCD panel
```

任选其一：

- `JD9165 1024x600`（默认）
- `EK79007 1024x600`

可选：在同一 **Display** 菜单中配置颜色格式（`RGB565` / `RGB888`）。

### 4. 编译 / 烧录 / 监视

**VS Code：** 使用状态栏按钮，或 `F1` 执行 `ESP-IDF: Build your project` / `Flash` / `Monitor`（也可一键 `Flash and start a monitor`）

**命令行：**

```bash
idf.py build
idf.py -p PORT flash monitor
```

将 `PORT` 替换为实际串口（Windows 如 `COM3`，Linux 如 `/dev/ttyUSB0`）。

退出监视器：`Ctrl+]`。

## 工程结构

```text
10_7inch_lvgl_demo_v9/
├── main/                         # 应用入口（LVGL Demo）
├── components/esp32_p4_s070_wba/ # 板级支持包（BSP）
├── common_components/bsp_extra/  # 扩展板级辅助
├── sdkconfig.defaults            # 工程默认配置
├── README.md                     # 英文说明
└── README_CN.md                  # 本文件（中文）
```

## 应用说明

- `main/main.c` 默认运行 `lv_demo_widgets()`。
- 可通过注释/取消注释切换 Demo，例如：
  - `lv_demo_music()`
  - `lv_demo_benchmark()`
  - `lv_demo_widgets()`
- 屏幕电源使能脚：**GPIO4**（1.8 V）、**GPIO5**（3.3 V），由 BSP 自动处理。

## 常见问题

| 现象 | 排查 |
|------|------|
| 无法启动 / 早期复位 | 检查芯片版本选项（见 3.1） |
| 花屏 / 无显示 | 检查 LCD 面板选择（见 3.2） |
| 链接时报 IRAM 溢出 | 保留 `sdkconfig.defaults` 中的 PSRAM XIP 与省 IRAM 配置 |

## 许可证

Apache-2.0（依赖组件以各自许可证为准）。
