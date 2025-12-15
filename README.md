# ESP32-P4-SmartDisplay
<img width="1158" height="694" alt="image" src="image/70E.png" />
<img width="1165" height="691" alt="image" src="image/70E-1.png" />

## 1.Introduction

The ESP32-P4-SmartDisplay is a high-performance development board equipped with a 7-inch MIPI screen (1024*600). It is designed by VIEWE based on the ESP32-P4 chip and ESP32-C6 module, supporting 2.4GHz Wi-Fi 6 and Bluetooth 5 (LE). The ESP32-P4 is equipped with a dual-core 360MHz RISC-V processor. This development board comes with multiple peripherals and interfaces: USB OTG 2.0 interface, MIPI-CSI interface, DSI interface, H.264 encoder, UART interface, RS485 interface, speaker interface, microphone, RGB light, SD card slot, etc., which fully meet the higher requirements of embedded applications in terms of human-machine interface support, edge computing capabilities, and IO connection characteristics. It can also meet customers' development needs for low-cost, high-performance, and low-power multimedia products.

### 1.1 Product Features
- Processor
  - Equipped with a RISC-V 32-bit dual-core processor (HP system), with DSP and instruction set extensions, floating-point arithmetic unit (FPU), and a main frequency of up to 400 MHz
    
  - Equipped with a RISC-V 32-bit single-core processor (LP system), with a main frequency of up to 40 MHz
    
  - Equipped with an ESP32-C6 WIFI/BT coprocessor, expanding functions such as WIFI 6/Bluetooth 5 through SDIO

- Memory
  - 128 KB of high-performance (HP) system read-only memory (ROM).
  - 16 KB of low-power (LP) system read-only memory (ROM).
  - 768 KB of high-performance (HP) L2 memory (L2MEM).
  - 32 KB of low-power (LP) SRAM.
  - 8 KB of system tightly coupled memory (TCM).
  - 32 MB PSRAM is stacked and sealed inside the package, and 16MB Nor Flash is connected through the QSPI interface

- Peripheral Interfaces
  - Two 2*20 Pin Headers are on-board to 34 programmable GPIOs, supporting a rich variety of peripheral devices
  - On-board SDIO3.0 SD card slot and Type-C UART programming port, facilitating use in different scenarios
  - On-board MIPI-CSI high-definition camera interface, supporting full HD 1080P video capture and encoding, integrating an image signal processor (ISP) and H264 video encoder, supporting H.264 & JPEG video encoding (1080P @30fps), facilitating applications in fields such as computer vision and machine vision
  - On-board 2 MIPI-DSI high-definition display interfaces, integrating a pixel processing accelerator (PPA) and 2D graphics acceleration controller (2D DMA), supporting JPEG image decoding (1080P @30fps), providing strong support for high-definition displays and smooth HMI experiences, facilitating applications in scenarios such as smart home control panels, industrial control panels, and vending machines

### 1.2 Applications

With low power consumption, ESP32-P4 is an ideal choice for IoT devices in the following areas:

• Smart Home

• Industrial Automation

• Health Care

• Consumer Electronics

• Smart Agriculture

• Retail Self-Service Terminals (POS, Vending Machines)

• Service Robot

• Multimedia Player

• Cameras for Video Streaming

• High-Speed USB Host and Device

• Smart Voice Interaction Terminal

• Edge Vision AI Processor

• HMI Control Pane

## 2. Hardware Description
### 2.1 Module Introduction
<img width="781" height="571" alt="image" src="image/70E-3.png" />

① ESP32-P4NRW32：
ESP32-P4 stacked with 32MB PSRAM

② ESP32-C6：
SDIO interface protocol, expanding ESP32-P4-SmartDisplay Wi-Fi 6 and Bluetooth 5

③ 7inch Display interface (MIPI 2-lane)：

4-DSI-TOUCH、
7-DSI-TOUCH、
10.1-DSI-TOUCH

④ 15pin Display interface (MIPI 2-lane)

⑤ 5B-MIPI Display interface

⑥ Universal Display Interface (MIPI 2-lane) 

⑦ Camera interface (MIPI 2-lane)

⑧ 7inch Touch interface

⑨ Type-C interface (USB2、USB3、UART)：
Can be used for power supply, program burning, and debugging，USB3 is a USB 2.0 full-speed OTG interface

⑩ RESET button

⑪ BOOT button：
Press when powering on or resetting to enter download mode

⑫ RS485
Industrial-grade serial communication standard

⑬ digital microphone（MSM2641D）

⑭ RGB-LED (WS2812B)

⑮ Speaker interface

⑯ TF card slot (SDIO 3.0)

⑰ P4 GPIO interface

⑱ C6 GPIO interface

⑲ USER-LED
Power indicator light

### 2.2 GPIO Definition
<img width="1509" height="1853" alt="pin_definition" src="image/pin_definition.png" />

### 2.3 GPIO Introduction
<img width="746" height="1382" alt="pin_introduction" src="image/pin_introduction.png" />

<img width="870" height="338" alt="module_color" src="image/module_color.png" />

## 3.Functional Block Diagram
The main components and connection methods of the ESP32-P4-SmartDisplay are shown in the following figure:

<img width="1088" height="617" alt="流程图" src="image/Flowchart.png" />

> [!NOTE]
> This board is the most basic version, and there are no external Ethernet.And we have also replaced the audio part, which consists of msm261d and ns4168. We will lead out the pin and can directly insert the expansion board later, and also reserve more creative possibilities for everyone

## 4.Instructions for Use
This tutorial aims to guide users to set up the software environment for ESP32-P4 hardware development, and demonstrates how to use the ESP-IDF configuration menu, compile, and download firmware to the ESP32-P4 development board through simple examples.

- Preparation
- Hardware
  - ESP32-P4-Pi-VIEWE Development Board
  - USB data cable (Type-A to Type-C, prepared as needed)
  - Computer (Windows, Linux or macOS)
- Software (It is recommended to install ESP-IDF using an integrated development environment. If you are familiar with ESP-IDF, you can start directly from the ESP-IDF terminal. You can choose any of the following development methods.)
  - VSCode + ESP-IDF plugin (recommended)
  - Eclipse + ESP-IDF plugin (Espressif-IDE)
  - Arduino IDE

## 5.Getting-start
### ESP-IDF
  - Please go to [ESP-IDF Quick Start](https://github.com/VIEWESMART/VIEWE-Tutorial/blob/main/esp-idf/esp-idf_Beginner_Tutorial.md) to see how to quickly set up the development environment and burn the application to your development board.
  - The application examples for the development board are stored in Examples. You can configure the project options by entering idf.py menuconfig in the [examples](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/tree/main/examples/esp-idf) directory.
### Arduino IDE
We are working hard to prepare. If you need anything, please contact us.

## 6.Related Documents
- [ESP32-P4-SmartDisplay Schematic Diagram (PDF)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/tree/main/schematic)
- [Camera Specification (PDF)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/peripheral/camera_datasheet.pdf)
- [Display Specification (PDF)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/display/HT070IBC-27N7EK-HD%2030PTT3558%20MiPi%2030%E7%9B%B4.pdf)
- [Display Chip Specification (PDF)]()
- [ESP32-P4-SmartDisplay Specification(PDF)]()
- [ESP32-C6 Datasheet(Chinese)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/chip/esp32-c6-wroom-1_wroom-1u_datasheet_cn.pdf)
- [ESP32-C6 Datasheet(English)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/chip/esp32-c6-wroom-1_wroom-1u_datasheet_en.pdf)
- [ESP32-P4 Datasheet (Chinese)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/chip/esp32-p4_datasheet_cn.pdf)
- [ESP32-P4 Datasheet (English)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/chip/esp32-p4_datasheet_en.pdf)
- [ESP32-P4 Technical Reference Manual (Chinese)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/chip/Esp32-p4_technical_reference_manual_cn.pdf)
- [ESP32-P4 Technical Reference Manual (English)](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/blob/main/datasheet/chip/Esp32-p4_technical_reference_manual_en.pdf)
- [Other Datasheet](https://github.com/VIEWESMART/ESP32-P4-SmartDisplay/tree/main/datasheet)

## Technical support

Contact person: VIEWE-Ayang

Email: smartrd1@viewedisplay.com

QQ technical exchange group: 1014311090

WeChat:
![wechat](image/wechat.jpg)

