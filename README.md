# AgentRing ESP32 LCD 蓝牙副屏 (AgentRing-ESP-LCD)

基于 **ESP32-P4** 与 **LVGL 9** 的桌面 AI 额度监控副屏固件。通过蓝牙接收并渲染 macOS 端 [AgentRing](https://github.com/davidhoo/agentRing) 的实时用量数据、同心圆环与重置倒计时，提供与 [agentRing-Android](https://github.com/davidhoo/agentRing-Android) 相同的高品质纯净信息展示。

---

## ✨ 核心特性

- **硬件支持**：针对微雪 **ESP32-P4-WIFI6-Touch-LCD-7B**（7 英寸 1024×600 IPS 电容触摸屏）量身定制，支持 32MB PSRAM 高性能平滑渲染。
- **协议 100% 对齐**：兼容 [BLUETOOTH_PROTOCOL.md](BLUETOOTH_PROTOCOL.md) 换行定界 JSON 流报文规范与心跳保活机制。
- **精美仪表盘排版**：
  - **顶部状态栏**：连接状态呼吸指示灯、设备广播名、最后数据刷新时间。
  - **横向多列弹性自适应**：Codex、Antigravity、Antigravity Third、Cursor 自动平铺。
  - **双同心 Activity Rings**：外环主额度 + 内环次额度，等比例粗弧线与纯净居中百分比大字。
  - **动态胶囊药丸**：严格对齐行高展示 `[ 额度标签 ]  [ 百分比/余额 ]  [ 倒计时 ]`。
  - **专属调色板**：精准匹配 macOS 原生配色。
- **无感重连与看门狗**：具备 60 秒链路超时检测与半开连接自动回收。

---

## 🛠️ 编译与烧录指南

### 1. 环境准备

确保已安装 **ESP-IDF v5.5.x**（推荐 v5.5.5）：

```bash
# 激活 ESP-IDF 编译环境
source ~/esp/esp-idf-v5.5/export.sh
```

### 2. 编译项目

进入本工程根目录：

```bash
cd ~/Code/github.com/agentRing-ESP-LCD

# 设置编译目标为 esp32p4
idf.py set-target esp32p4

# 编译项目（自动拉取微雪 BSP 与 LVGL 9 组件）
idf.py build
```

### 3. 烧录与日志监视

将开发板通过 USB-C 接口连接至 Mac：

```bash
# 查找串口设备（通常为 /dev/cu.usbserial-* 或 /dev/cu.usbmodem*）
ls /dev/cu.usb*

# 烧录固件并打开串口监视器
idf.py -p /dev/cu.usbmodemXXXX flash monitor
```

---

## 📡 蓝牙通讯机制

1. **设备广播名**：`AgentRing-ESP32-LCD`。
2. **报文协议**：单行 UTF-8 JSON 文本，以换行符 `\n` 作为定界符。
3. **心跳帧**：支持接收远端 `{"type":"ping","timestamp":...}` 并重置超时计数器。

---

## 📄 许可证

本项目基于 Apache License 2.0 授权开源。
