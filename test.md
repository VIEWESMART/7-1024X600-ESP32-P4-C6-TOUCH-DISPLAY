# ESP32-P4-WIFI6 7 Inch 1024x600 Smart Display

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2%2B-red)](https://github.com/espressif/esp-idf)

[中文版本](./README_CN.md) | [English](./README.md)

<div align="center">
  <img src="image/70E.png" width="45%" alt="Front View">
  <img src="image/70E-1.png" width="45%" alt="Back View">
</div>

## 1. Introduction

The **ESP32-P4-SmartDisplay** is a high-performance development board equipped with a 7-inch MIPI screen (1024x600). Designed by VIEWE, it combines the **ESP32-P4** (High-Performance MCU) with the **ESP32-C6** (Wi-Fi 6 + BT 5 Module).

This board integrates rich peripherals including USB OTG 2.0, MIPI-CSI Camera, H.264 Encoder, and Industrial Interfaces (RS485), making it an ideal platform for **HMI**, **Edge Computing**, and **Multimedia** applications.

### 1.1 Product Features
* **Processor**:
    * **ESP32-P4**: RISC-V Dual-Core HP @ 400MHz + LP Core @ 40MHz.
    * **ESP32-C6**: Wi-Fi 6 / BLE 5.0 Co-processor (via SDIO).
* **Memory**:
    * 32MB PSRAM (Stacked), 16MB NOR Flash.
* **Multimedia**:
    * **Display**: 7-inch IPS (1024x600) via MIPI-DSI (2-Lane).
    * **Camera**: MIPI-CSI Interface (Support 1080P @ 30fps).
    * **Video**: Hardware H.264 Encoding / JPEG Decoding.
    * **Audio**: Digital Mic (MSM2641D) + Speaker Amp.

### 1.2 Applications
* Smart Home Control Panels
* Industrial HMI & Automation
* Edge AI Vision & Multimedia Players

---

## 2. Hardware Description

### 2.1 Module Overview
The detailed component layout is shown below:

![Board Layout](image/70E-3.png)

| No. | Component | Description |
| :--- | :--- | :--- |
| **①** | **ESP32-P4NRW32** | Main SoC (Stacked 32MB PSRAM). |
| **②** | **ESP32-C6** | Wi-Fi 6 / Bluetooth 5 Co-processor (SDIO). |
| **③** | **Display Header** | MIPI-DSI 2-Lane (Compatible with VIEWE 4"/7"/10.1" Screens). |
| **④** | **15-Pin FPC** | Generic MIPI-DSI Interface. |
| **⑤** | **5B-MIPI** | Alternative Display Interface. |
| **⑥** | **Universal MIPI** | General Purpose MIPI Connector. |
| **⑦** | **Camera Interface** | MIPI-CSI 2-Lane (Supports 1080P Input). |
| **⑧** | **Touch Interface** | I2C Capacitive Touch Header (7-inch). |
| **⑨** | **USB Type-C** | **Power / UART / USB OTG 2.0**. |
| **⑩** | **RESET Button** | Hardware Reset. |
| **⑪** | **BOOT Button** | Press during power-on to enter Download Mode. |
| **⑫** | **RS485** | Industrial Serial Interface (Transceiver onboard). |
| **⑬** | **Mic** | Digital Microphone (MSM2641D). |
| **⑭** | **RGB LED** | WS2812B Addressable LED. |
| **⑮** | **Speaker** | Connector for External Speaker (NS4168 Amp). |
| **⑯** | **TF Card Slot** | SDIO 3.0 Interface. |
| **⑰** | **P4 GPIO** | Breakout header for ESP32-P4 pins. |
| **⑱** | **C6 GPIO** | Breakout header for ESP32-C6 pins. |
| **⑲** | **Power LED** | 5V Power Indicator. |

### 2.2 GPIO Definition (Pinout)
The complete pin mapping for the 2x20 headers:

![Pin Definition](image/pin_definition.png)

### 2.3 GPIO Function Details
Detailed function list for P4 and C6 GPIOs:

![Pin Introduction](image/pin_introduction.png)

> **LED Indicator Color Code:**
> ![Module Color](image/module_color.png)

### 2.4 Mechanical Dimensions
Physical size and mounting hole positions:

![Dimensions](image/size.png)

---

## 3. Functional Block Diagram

The system architecture and connection between ESP32-P4 (Master) and ESP32-C6 (Slave) are illustrated below:

![Flowchart](image/Flowchart.png)

> [!NOTE]
> **Hardware Version Info**:
> This board is the standard version. It does not include an onboard Ethernet PHY (can be expanded via SPI/RMII).
> The audio circuit uses **MSM261D** (Mic) and **NS4168** (Amp).

---

## 4. Instructions for Use (Software)

### Preparation
* **Hardware**: ESP32-P4-SmartDisplay Board, USB-C Cable.
* **Software**: **ESP-IDF v5.2** or later (Required).

### Getting Started

1.  **Clone the Repository**
    ```bash
    git clone [https://github.com/VIEWESMART/ESP32-P4-SmartDisplay.git](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay.git)
    ```

2.  **Set Target**
    ```bash
    idf.py set-target esp32p4
    ```

3.  **Wi-Fi Configuration (Important)**
    Since P4 uses C6 for Wi-Fi via SDIO, you must add these dependencies:
    ```bash
    idf.py add-dependency "espressif/esp_wifi_remote"
    idf.py add-dependency "espressif/esp_hosted"
    ```

4.  **Build & Flash**
    ```bash
    idf.py build flash monitor
    ```

> For Arduino support, please stay tuned for updates.

---

## 5. Related Documents & Resources

All datasheets, schematics, and reference manuals can be downloaded directly from the table below.

### 📄 Board Documents
| Document | Description |
| :--- | :--- |
| **[Schematic Diagram](schematic/ESP32-P4-SmartDisplay.sch.pdf)** | Circuit Design & PCB Connections |
| **[Board Specification]()** | ESP32-P4-SmartDisplay Product Spec (PDF) |
| **[Display Specification](datasheet/display/HT070IBC-27N7EK-HD%2030PTT3558%20MiPi%2030%E7%9B%B4.pdf)** | 7.0" 1024x600 IPS Panel Spec |
| **[Display IC Datasheet](datasheet/display/EK79007AD3_DS_REV1.0(1).pdf)** | EK79007AD3 Driver IC Manual |
| **[Camera Specification](datasheet/peripheral/camera_datasheet.pdf)** | MIPI-CSI Camera Module Spec |

### 🧠 Chip Datasheets (Espressif)
| Chip | Document | Language |
| :--- | :--- | :--- |
| **ESP32-P4** | [Datasheet](datasheet/chip/esp32-p4_datasheet_en.pdf) | English |
| | [Datasheet](datasheet/chip/esp32-p4_datasheet_cn.pdf) | Chinese |
| | [Tech Reference Manual](datasheet/chip/Esp32-p4_technical_reference_manual_en.pdf) | English |
| | [Tech Reference Manual](datasheet/chip/Esp32-p4_technical_reference_manual_cn.pdf) | Chinese |
| **ESP32-C6** | [Datasheet](datasheet/chip/esp32-c6-wroom-1_wroom-1u_datasheet_en.pdf) | English |
| | [Datasheet](datasheet/chip/esp32-c6-wroom-1_wroom-1u_datasheet_cn.pdf) | Chinese |

> [!TIP]
> For more resources, please visit the [datasheet/](/datasheet) folder in this repository.

---

## 📞 Technical Support

* **Issues**: [GitHub Issues](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/issues)
* **Email**: smartrd1@viewedisplay.com
* **Community**:
    * QQ Group: 1014311090
    * WeChat: (Scan QR code in `image/wechat.jpg`)<img width="1045" height="291" alt="7688eff8-03ea-4d67-9b6c-936fecafe98d"
