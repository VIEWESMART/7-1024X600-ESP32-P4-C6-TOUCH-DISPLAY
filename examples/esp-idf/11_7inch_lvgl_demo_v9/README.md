# ESP32-P4-S070-WBA LVGL Demo (v9)

LVGL 9 demo for the **ESP32-P4-S070-WBA** development board with a 7-inch **1024×600** MIPI DSI display.

Supported LCD panels (selectable in `menuconfig`):

| Panel | Resolution |
|-------|------------|
| JD9165 | 1024×600 |
| EK79007 | 1024×600 |

## Requirements

- ESP-IDF **v5.4+** (tested with v5.5.x)
- Target: **ESP32-P4**
- Board: ESP32-P4-S070-WBA with PSRAM

## Quick Start

### 1. Set up ESP-IDF

Use either method below.

#### Option A: VS Code + ESP-IDF extension (recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the **Espressif IDF** extension from the marketplace (publisher: Espressif Systems)
3. Press `F1`, run `ESP-IDF: Configure ESP-IDF Extension`, and complete the setup wizard (install IDF/toolchain or select an existing path)
4. Open this project folder `10_7inch_lvgl_demo_v9` in VS Code
5. Confirm the target chip is **esp32p4** in the status bar / command palette, and select the correct serial port (e.g. `COM3`)

Then use the VS Code status bar or command palette:

| Action | Command |
|--------|---------|
| Set target | `ESP-IDF: Set Espressif Device Target` → `esp32p4` |
| menuconfig | `ESP-IDF: SDK Configuration Editor (menuconfig)` |
| Build | `ESP-IDF: Build your project` |
| Flash | `ESP-IDF: Flash your project` |
| Monitor | `ESP-IDF: Monitor your device` |
| Flash + monitor | `ESP-IDF: Flash and start a monitor on a device` |

> If the extension reports a missing environment, run `ESP-IDF: Configure ESP-IDF Extension` again.

#### Option B: Command line

```bash
# Linux / macOS
. $HOME/esp/esp-idf/export.sh

# Windows (ESP-IDF PowerShell / CMD)
export.bat
# or: . .\export.ps1
```

### 2. Configure the target

**VS Code:** `F1` → `ESP-IDF: Set Espressif Device Target` → select `esp32p4`

**CLI:**

```bash
cd 10_7inch_lvgl_demo_v9
idf.py set-target esp32p4
```

### 3. Open menuconfig

**VS Code:** `F1` → `ESP-IDF: SDK Configuration Editor (menuconfig)`

**CLI:**

```bash
idf.py menuconfig
```

#### 3.1 Chip revision (important)

Go to:

```text
(Top) → Component config → Hardware Settings → Chip revision
→ Select ESP32-P4 revisions <3.0 (No >=3.x Support)
```

| Your ESP32-P4 silicon | Action |
|-----------------------|--------|
| Revision **below 3.0** (e.g. 0.x / 1.x) | **Enable** this option |
| Revision **3.0 or above** | **Disable** this option |

> Using the wrong chip-revision setting may cause boot failure or unstable behavior.

#### 3.2 Select LCD panel

Go to:

```text
(Top) → Board Support Package (ESP32-P4-S070-WBA) → Display → Select LCD panel
```

Choose one of:

- `JD9165 1024x600` (default)
- `EK79007 1024x600`

Optional: color format under the same **Display** menu (`RGB565` / `RGB888`).

### 4. Build / flash / monitor

**VS Code:** Use the status-bar buttons, or press `F1` and run `ESP-IDF: Build your project` / `Flash` / `Monitor` (or `Flash and start a monitor`)

**CLI:**

```bash
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your serial port (e.g. `COM3` on Windows, `/dev/ttyUSB0` on Linux).

Exit the monitor with `Ctrl+]`.

## Project Structure

```text
10_7inch_lvgl_demo_v9/
├── main/                         # Application entry (LVGL demos)
├── components/esp32_p4_s070_wba/ # Board Support Package (BSP)
├── common_components/bsp_extra/  # Extra board helpers
├── sdkconfig.defaults            # Default project configuration
├── README.md                     # This file (English)
└── README_CN.md                  # Chinese version
```

## Application Notes

- Default demo in `main/main.c` is `lv_demo_widgets()`.
- You can switch demos by commenting/uncommenting calls such as:
  - `lv_demo_music()`
  - `lv_demo_benchmark()`
  - `lv_demo_widgets()`
- Display power enables: **GPIO4** (1.8 V) and **GPIO5** (3.3 V), handled by the BSP.

## Troubleshooting

| Symptom | Check |
|---------|--------|
| Won't boot / resets early | Chip revision option (section 3.1) |
| Blank / wrong screen | LCD panel selection (section 3.2) |
| Link IRAM overflow | Keep PSRAM XIP and IRAM-saving options from `sdkconfig.defaults` |

## License

Apache-2.0 (see component / ESP-IDF licenses for dependencies).
