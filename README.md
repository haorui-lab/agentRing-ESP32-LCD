# AgentRing ESP32 LCD 桌面副屏 (AgentRing-ESP32-LCD)

<p align="center">
  <img src="https://img.shields.io/badge/Hardware-ESP32--P4%20%2B%20ESP32--C6-E7352C?style=flat-square&logo=espressif" alt="Hardware">
  <img src="https://img.shields.io/badge/Display-1024x600%20IPS%20MIPI%20DSI-blue?style=flat-square" alt="Display">
  <img src="https://img.shields.io/badge/GUI-LVGL%20v9-orange?style=flat-square&logo=lvgl" alt="LVGL">
  <img src="https://img.shields.io/badge/Framework-ESP--IDF%20v5.5-red?style=flat-square" alt="ESP-IDF">
  <img src="https://img.shields.io/badge/Protocol-BLE%205.0%20GATT%20%2F%20USB--C-success?style=flat-square&logo=bluetooth" alt="Protocol">
  <img src="https://img.shields.io/badge/License-Apache%202.0-blue?style=flat-square" alt="License">
</p>

基于 **ESP32-P4** 高性能双核 RISC-V 芯片与 **LVGL 9** 打造的独立桌面硬件副屏固件。通过低功耗蓝牙 (BLE 5.0 GATT) 或 USB-C 串口，无缝直连 macOS 状态栏应用 **[AgentRing](https://github.com/davidhoo/agentRing)**，实时呈现各大主流 AI 编程助手（**Codex / Antigravity / Claude / GPT / Cursor**）的额度同心圆环、剩余百分比与重置倒计时。

遵循 Apple HIG 与 Apple Watch Activity Rings 设计语言，提供清爽无扰、通电即连、零云端中转的硬件级极客桌面摆件。

---

## 🧭 项目定位与生态关系

本项目是 **AgentRing 生态体系** 中的**专属嵌入式硬件副屏端**：

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           macOS 核心中枢                                │
│                     AgentRing (macOS 状态栏应用)                         │
│                                                                         │
│   • 聚合抓取 Codex / Cursor / Antigravity / Claude 等本地与云端额度      │
│   • 计算剩余百分比、多窗口（5小时/7天）重置时间                          │
│   • BLESyncService: CoreBluetooth 后台自发现并秒连副屏                  │
│   • 定时与事件触发推送单行 JSON 报文 + 15s 心跳保活                     │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
                 ┌───────────────────┴───────────────────┐
                 │ 无线 BLE 5.0 (GATT) / 有线 USB-C      │ 经典蓝牙 SPP
                 ▼                                       ▼
┌───────────────────────────────────┐   ┌─────────────────────────────────┐
│   AgentRing-ESP32-LCD (本项目)    │   │       agentRing-Android         │
│                                   │   │                                 │
│ • 微雪 7寸 1024x600 IPS 电容触摸屏 │   │ • 闲置 Android 手机 / 平板方案  │
│ • ESP32-P4 + C6 协处理器 (免OS)    │   │ • 电池驱动便携屏                │
│ • 极速冷启动、微秒级响应、超低功耗 │   └─────────────────────────────────┘
│ • 纯粹桌面极客硬件摆件            │
└───────────────────────────────────┘
```

- **与 macOS [AgentRing](https://github.com/davidhoo/agentRing) 的关系**：
  - **AgentRing (Mac)** 是**数据源与主控端**，负责与各大 AI Provider 交互，计算最新用量。
  - **AgentRing-ESP32-LCD** 是**纯净展示终端**，板端无需配置任何 API Key，也无需连接外网 Wi-Fi，所有数据均来自 Mac 本地通过蓝牙/串口的加密直连推流。
- **与 [agentRing-Android](https://github.com/davidhoo/agentRing-Android) 的关系**：
  - 两者共享完全相同的 **视觉设计规范与数据协议标准**（圆环内外等宽、移除中心干扰数字、三通道扁平告急变色行、macOS 浅灰护眼底色）；
  - `agentRing-Android` 面向利用旧手机、旧平板打造副屏；
  - `AgentRing-ESP32-LCD` 面向极致的**独立专用微控制器硬件**，无操作系统开销、通电秒开、长期插电发热极低、寿命长久。

---

## ✨ 核心特性

- 🖥️ **顶级硬件表现**：
  - 针对微雪 **ESP32-P4-WIFI6-Touch-LCD-7B** 量身适配（7 英寸 1024×600 IPS 全贴合液晶屏、EK79007 MIPI DSI 控制器、GT911 电容触控）。
  - 双核 RISC-V @ 400MHz + 32MB 高速 PSRAM，LVGL 9 双缓冲直接驱动，动效丝滑无撕裂。
- 🎯 **设计质感 1:1 对齐 Apple HIG**：
  - **纯净留白同心圆环**：彻底摒弃圆环中央堆砌文字的粗糙感，内外环留白通透。
  - **双环等宽饱满规范**：单窗口供应商呈现单圈大环，双窗口（如 Antigravity 5小时 + 7天）呈现同心内外双环，内外环均为等宽 18px 饱满线宽与 4.5% 呼吸间隙。
  - **三通道扁平明细行**：左侧额度名、中间加粗百分比（支持用量告急分级着色：正常 `#111827`、≤20% 警告橙 `#EA580C`、≤5% 严重告急红 `#DC2626`）、右侧重置倒计时。
  - **macOS 原生底色**：采用柔和的浅灰护眼底色（`#F5F6F8`），搭配轻量级顶部状态栏与底部同步说明栏。
- ⚡ **真正的“零配置、通电即连”**：
  - 采用 **纯低功耗蓝牙 (BLE 5.0 GATT)** 广播标准 Nordic UART 串口服务；
  - 用户**完全无需在 macOS 系统设置中手动配对或输 PIN 码**。开发板通电后，Mac 端的 AgentRing 在后台自动发现并于 1 秒内握手推流；
  - 内置 FreeRTOS RingBuffer 异步解耦机制，保证高频传输零掉包、零栈溢出、超长稳定运行。
- 🔌 **无线 BLE + 有线 USB-C 双模支持**：
  - 默认采用 BLE 5.0 无线通信，桌面摆放极简清爽；
  - 亦可直接插入 USB-C 数据线，自动启用 USB 虚拟串口收发，无需蓝牙也可即插即用。
- 💡 **下拉触控控制中心（背光亮度无级调节）**：
  - 点击顶部栏快捷胶囊 `[ 亮度 100% ]` 或轻触状态栏任意区域，即刻呼出 macOS 控制中心质感的浮动控制面板；
  - 支持 **10% ~ 100% 连续触控滑动条**（硬件 LEDC PWM 平滑调光），安全限制最低 10% 亮度杜绝意外黑屏；
  - 贴心配备 **4 档一键预设**（`25% 低亮`、`50% 中亮`、`75% 高亮`、`100% 极亮`）；
  - 基于 ESP32 内部 NVS 掉电非易失性存储，断电重启自动恢复背光档位；轻点屏幕背景空白处或右上角关闭按钮随时收起。
- 🔒 **100% 隐私安全与去中心化**：
  - 全流程局域网点对点通信，无任何第三方服务器接入，无云端中转，杜绝 API Token 泄露风险。

---

## 📦 硬件清单与选型

| 部件 | 推荐型号 / 规格 | 说明 |
| :--- | :--- | :--- |
| **核心板卡** | **Waveshare ESP32-P4-WIFI6-Touch-LCD-7B** | 集成 ESP32-P4、ESP32-C6 协处理器、32MB PSRAM、Type-C 供电/调试口 |
| **屏幕** | 7 英寸 1024×600 IPS 电容触摸屏（自带） | MIPI DSI 接口，全视角高亮显示 |
| **供电/数据线** | 标准 USB Type-C 数据线 | 5V / 1A 即可稳定驱动 |
| **支架/外壳** | 桌面倾斜亚克力支架或 3D 打印外壳 | 舒适仰角适合桌边摆放 |

---

## 🚀 用户快速上手指南

### 第一步：固件烧录

如果您已从 Release 下载了预编译固件，或者通过源码自行编译：

```bash
# 进入工程目录并激活环境
source ~/esp/esp-idf-v5.5/export.sh

# 一键编译并烧录至已连接的开发板 (请替换对应串口端口)
idf.py -p /dev/cu.usbmodem* flash
```

### 第二步：开启 Mac 端 AgentRing

1. 在 Mac 上运行最新版 **[AgentRing](https://github.com/davidhoo/agentRing)**；
2. 确保在菜单栏 **AgentRing -> 偏好设置** 中勾选开启了 **“启用副屏蓝牙同步”**；
3. **完成！** 无需在 macOS“系统设置 -> 蓝牙”里手动配对搜索。开发板上电后，Mac 端会自动发现广播名 `AgentRing-ESP32-LCD`，屏幕将自动从“等待 AgentRing 同步”卡片平滑切换为实时 4 列仪表盘。

> [!TIP]
> **常见疑问：为什么在 macOS 系统设置的蓝牙设备列表里看不到它？**
> macOS 系统设置仅罗列传统经典蓝牙（Audio、键盘鼠标等 HID）或已进行系统级绑定的设备。本项目采用纯 BLE 5.0 GATT 通信，连接完全由 AgentRing 应用层（CoreBluetooth）在沙盒内透明管理，这种设计免去了复杂的配对确认弹窗，实现开箱即连。

---

## 🛠️ 开发者指南 (编译与调试)

### 1. 环境准备

推荐使用官方 **ESP-IDF v5.5.x** 工具链：

```bash
# 激活 ESP-IDF 编译环境
. $HOME/esp/esp-idf-v5.5/export.sh
```

### 2. 源码构建

```bash
git clone https://github.com/haorui-lab/agentRing-ESP32-LCD.git
cd agentRing-ESP32-LCD

# 设置目标芯片为 esp32p4
idf.py set-target esp32p4

# 编译项目 (自动下载 esp-bsp 与 LVGL 9 等依赖组件)
idf.py build
```

### 3. 烧录与串口实时调试

```bash
# 烧录并进入实时日志监视
idf.py -p /dev/cu.usbmodem* flash monitor
```

---

## 📡 通信协议与数据格式

### 1. 蓝牙 GATT 特性

- **Service UUID**：`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`（标准 Nordic UART）
- **RX Characteristic (Write Without Response)**：`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX Characteristic (Notify)**：`6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- **MTU**：支持 256 字节动态协商

### 2. 报文示例 (换行符 `\n` 定界)

#### 数据帧示例
```json
{
  "timestamp": 1789896924,
  "providers": [
    {
      "id": "codex",
      "name": "Codex",
      "primary": { "label": "7天额度", "remainingPercent": 95.0, "resetsAt": "3d 12h" },
      "rows": [
        { "label": "7天额度", "percent": "95%", "reset": "3d 12h" }
      ]
    },
    {
      "id": "antigravity",
      "name": "Antigravity",
      "primary": { "label": "Gemini 5小时", "remainingPercent": 78.0, "resetsAt": "2h 15m" },
      "secondary": { "label": "Gemini 7天", "remainingPercent": 91.0, "resetsAt": "5d 18h" },
      "rows": [
        { "label": "Gemini 5小时", "percent": "78%", "reset": "2h 15m" },
        { "label": "Gemini 7天", "percent": "91%", "reset": "5d 18h" }
      ]
    },
    {
      "id": "antigravity_third",
      "name": "Claude / GPT",
      "primary": { "label": "Claude 5小时", "remainingPercent": 45.0, "resetsAt": "1h 30m" },
      "secondary": { "label": "Claude 7天", "remainingPercent": 82.0, "resetsAt": "4d 6h" },
      "rows": [
        { "label": "Claude 5小时", "percent": "45%", "reset": "1h 30m" },
        { "label": "Claude 7天", "percent": "82%", "reset": "4d 6h" }
      ]
    },
    {
      "id": "cursor",
      "name": "Cursor",
      "primary": { "label": "Cursor 模型", "remainingPercent": 76.0, "resetsAt": "11d 8h" },
      "rows": [
        { "label": "Cursor 模型", "percent": "76%", "reset": "11d 8h" },
        { "label": "其他模型", "percent": "$25.00", "reset": "" }
      ]
    }
  ]
}
```

#### 心跳保活帧 (15s 周期)
```json
{"type":"ping","timestamp":1789896939}
```

---

## 📂 代码架构一览

```
agentRing-ESP-LCD/
├── main/
│   ├── main.c                   # 系统入口、NVS 与屏幕 BSP 初始化、主循环
│   ├── bt/
│   │   ├── ble_server.c/h       # NimBLE GATT 外设服务端实现与 MTU 协商
│   │   └── bt_transport.c/h     # FreeRTOS 16KB RingBuffer 异步解耦传输层、USB-C 串口兜底
│   ├── model/
│   │   └── sync_payload.c/h     # cJSON 健壮状态机解析器、前导杂波过滤、数据模型
│   └── ui/
│       ├── ui_dashboard.c/h     # LVGL 9 仪表盘界面、Apple 规范同心双环、扁平三通道明细行
│       ├── ui_theme.c/h         # 专属调色板、额度告急分级动态着色器
│       └── ui_font_chinese_*.c  # 4bpp 无损平滑中文点阵字体
├── scripts/
│   └── feed_live_data.py        # 离线或自动化推送测试工具 (经 USB 串口验证)
├── CMakeLists.txt
├── idf_component.yml            # 组件依赖配置 (esp_bsp_generic, lvgl)
├── sdkconfig.defaults           # NimBLE、ESP-Hosted SDIO 协处理器驱动配置
└── README.md
```

---

## 🤝 参与贡献

欢迎通过 Issue 或 Pull Request 共同完善本项目！
- 适配其他尺寸与接口的 ESP32 开发板（如 3.5 寸、4.3 寸 RGB/SPI 屏幕或 ESP32-S3 系列）；
- 丰富交互手势（电容屏点击切换视图、查看详细按量折线图等）；
- 提交更丰富的 3D 打印外壳模型。

---

## 📄 开源许可证

本项目基于 [Apache License 2.0](LICENSE) 开源发布。
配套的 macOS 客户端及 Android 副屏端请参阅其对应代码仓库。
