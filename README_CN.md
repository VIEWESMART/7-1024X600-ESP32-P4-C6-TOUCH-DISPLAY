# ESP32-P4-SmartDisplay
<img width="1158" height="694" alt="image" src="image/70E.png" />
<img width="1165" height="691" alt="image" src="image/70E-1.png" />

## 1. 简介
ESP32-P4-SmartDisplay 是一款高性能开发板，配备 7 英寸 MIPI 屏幕（1024*600）。它由 VIEWE 基于 ESP32-P4 芯片和 ESP32-C6 模块设计，支持 2.4GHz Wi-Fi 6 和蓝牙 5（LE）。ESP32-P4 搭载了双核 360MHz RISC-V 处理器。这款开发板配备了多种外设和接口：USB OTG 2.0 接口、MIPI-CSI 接口、DSI 接口、H.264 编码器、UART 接口、RS485 接口、扬声器接口、麦克风、RGB 灯、SD 卡槽等，能充分满足嵌入式应用在人机界面支持、边缘计算能力和 IO 连接特性方面的更高要求。它还能满足客户对低成本、高性能、低功耗多媒体产品的开发需求。

### 1.1 产品特点
- 处理器
  - 配备 RISC-V 32 位双核处理器（HP 系统），具有 DSP 和指令集扩展、浮点运算单元（FPU），主频高达 400 MHz
  - 配备 RISC-V 32 位单核处理器（LP 系统），主频高达 40 MHz
  - 配备 ESP32-C6 Wi-Fi/蓝牙协处理器，通过 SDIO 扩展 Wi-Fi 6/蓝牙 5 等功能
- 内存
  - 128 KB高性能（HP）系统只读存储器（ROM）。
  - 16 KB低功耗（LP）系统只读存储器（ROM）。
  - 768 KB高性能（HP）二级内存（L2MEM）。
  - 32 KB低功耗（LP）静态随机存取存储器（SRAM）。
  - 8 KB系统紧耦合存储器（TCM）。
  - 32 MB伪静态随机存取存储器（PSRAM）堆叠并密封在封装内部，16 MB NOR闪存通过QSPI接口连接
- 外设接口
  - 板载两个2*20引脚接头，可提供34个可编程通用输入/输出接口（GPIOs），支持多种外围设备
  - 板载SDIO3.0 SD卡插槽和Type-C通用异步收发传输器（UART）编程端口，便于在不同场景中使用
  - 板载移动行业处理器接口-摄像头串行接口（MIPI-CSI）高清摄像头接口，支持全高清1080P视频采集和编码，集成图像信号处理器（ISP）和H264视频编码器，支持H.264和JPEG视频编码（1080P@30fps），便于在计算机视觉和机器视觉等领域应用
  - 板载2个移动行业处理器接口-显示串行接口（MIPI-DSI）高清显示接口，集成像素处理加速器（PPA）和2D图形加速控制器（2D DMA），支持JPEG图像解码（1080P@30fps），为高清显示和流畅的人机界面（HMI）体验提供有力支持，便于在智能家居控制面板、工业控制面板和自动售货机等场景中应用
### 1.2 应用领域
ESP32-P4功耗低，是以下领域物联网设备的理想选择：
• 智能家居

• 工业自动化

• 医疗健康

• 消费电子

• 智慧农业

• 零售自助终端（POS机、自动售货机）

• 服务机器人

• 多媒体播放器

• 视频流摄像机

• 高速USB主机及设备

• 智能语音交互终端

• 边缘视觉人工智能处理器

• 人机界面控制面板

## 2. 硬件描述
### 2.1 模块介绍
<img width="781" height="571" alt="image" src="image/70E-3.png" />
① ESP32-P4NRW32：
ESP32-P4 搭载 32MB PSRAM

② ESP32-C6：
SDIO接口协议，扩展ESP32-P4-SmartDisplay的Wi-Fi 6和蓝牙5功能

③ 7英寸显示屏接口（MIPI 2通道）：
4-DSI-触摸、
7-DSI-触摸、
10.1-DSI-触摸

④ 15针显示屏接口（MIPI 2通道）

⑤ 5B-MIPI显示屏接口

⑥ 通用显示屏接口（MIPI 2通道）

⑦ 摄像头接口（MIPI 2通道）

⑧ 7英寸触摸接口

⑨ Type-C接口（USB2、USB3、UART）：
可用于供电、程序烧录和调试，USB3是USB 2.0全速OTG接口

⑩ 复位按钮

⑪ 启动按钮：
开机或复位时按下可进入下载模式

⑫  RS485
工业级串行通信标准

⑬ 数字麦克风（MSM2641D）

⑭ RGB发光二极管（WS2812B）

⑮ 扬声器接口

⑯ TF卡插槽（SDIO 3.0）

⑰ P4通用输入输出接口

⑱ C6通用输入输出接口

⑲ 用户发光二极管:
电源指示灯

### 2.2 GPIO定义
<img width="1509" height="1853" alt="pin_definition" src="image/pin_definition.png" />

### 2.3 GPIO简介
<img width="746" height="1382" alt="pin_introduction" src="image/pin_introduction.png" />

<img width="870" height="338" alt="module_color" src="image/module_color.png" />

## 3.功能框图
ESP32-P4-SmartDisplay的主要组件及连接方式如下图所示：

<img width="1088" height="617" alt="流程图" src="image/Flowchart.png" />

> [!NOTE]
> 这块电路板是基础的版本，没有外部以太网接口。而且我们还更换了音频部分，它由msm261d和ns4168组成。我们会将引脚引出，之后可以直接插入扩展板，也为大家预留了更多创意的可能。

## 4.使用说明
本教程旨在指导用户搭建用于ESP32-P4硬件开发的软件环境，并通过简单示例演示如何使用ESP-IDF配置菜单、编译固件以及将固件下载到ESP32-P4开发板。

- 准备工作
- 硬件
  - ESP32-P4-Pi-VIEWE开发板
  - USB数据线（Type-A转Type-C，根据需要准备）
  - 计算机（Windows、Linux或macOS系统）
- 软件（建议使用集成开发环境安装ESP-IDF。如果您熟悉ESP-IDF，可以直接从ESP-IDF终端开始。您可以选择以下任意一种开发方式。）
  - VSCode + ESP-IDF插件（推荐）
  - Eclipse + ESP-IDF插件（Espressif-IDE）
  - Arduino IDE

## 5.入门指南
### ESP-IDF
- 新手请前往[ESP-IDF快速入门](https://github.com/VIEWESMART/VIEWE-Tutorial/blob/main/esp-idf/esp-idf_Beginner_Tutorial.md)查看如何快速搭建开发环境并将应用程序烧录到开发板。
- 开发板的应用示例存储在Examples文件夹中。您可以在[examples](examples/esp-idf)，其中包含了使用说明，如未添加请耐心等待我们在逐一添加中，也可联系我们，我们会优先处理

## 6.Related Documents
- [ESP32-P4-SmartDisplay原理图（PDF）](schematic/ESP32-P4-SmartDisplay.sch.pdf)
- [相机规格（PDF）](datasheet/peripheral/camera_datasheet.pdf)
- [显示规格（PDF）](datasheet/display/HT070IBC-27N7EK-HD%2030PTT3558%20MiPi%2030%E7%9B%B4.pdf)
- [显示芯片规格（PDF）](datasheet/display/EK79007AD3_DS_REV1.0(1).pdf)
- [ESP32-P4-SmartDisplay 规格书(PDF)]()
- [ESP32-C6 数据手册（中文）](datasheet/chip/esp32-c6-wroom-1_wroom-1u_datasheet_cn.pdf)
- [ESP32-C6 数据手册（英文）](datasheet/chip/esp32-c6-wroom-1_wroom-1u_datasheet_en.pdf)
- [ESP32-P4 数据手册（中文）](datasheet/chip/esp32-p4_datasheet_cn.pdf)
- [ESP32-P4 数据手册（英文）](datasheet/chip/esp32-p4_datasheet_en.pdf)
- [ESP32-P4 技术参考手册（中文）](datasheet/chip/Esp32-p4_technical_reference_manual_cn.pdf)
- [ESP32-P4 技术参考手册 (英文)](datasheet/chip/Esp32-p4_technical_reference_manual_en.pdf)
- [其他数据手册](datasheet)

## Technical support

联系人：VIEWE - Ayang

邮箱：smartrd1@viewedisplay.com

QQ技术交流群：1014311090

微信：
![wechat](image/wechat.jpg)

