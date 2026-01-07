EGoTouch-Rev
======================

**EGoTouch-Rev** 是针对 **Matebook E Go (Windows on Arm64)** 设备触摸屏驱动服务的高性能重构项目。

## ⚠️ 维护状态声明

> **注意：** 项目开发者目前专注于考试，近期无暇继续该项目的维护与开发。
>
> 本项目目前处于**早期开发阶段 (Pre-Alpha)**，仅供有经验的开发者学习研究。

## 📚 技术文档

关于详细的逆向工程发现（如 Master/Slave 帧结构解析）和技术路线分析，请务必阅读：
👉 [**docs/TECHNICAL_DETAILS.md (技术细节报告)**](docs/TECHNICAL_DETAILS.md)

-------

## 项目目标

- 替代原厂效率较低的 `HuaweiThpService.exe`，通过原生 C++23 直接与底层 SPI 驱动通信。
- 探索更低延迟的触摸响应方案。
- 解决或优化原厂驱动的“鬼影”触控问题。

## 核心硬件与架构

* **目标设备**: Matebook E Go (2022性能版)
* **Touch IC**: Himax HX83121A (SPI Interface)
* **架构设计**:
  * **User Mode**: `EGoTouchService` (本服务) - 负责 SPI 通信、协议解析。
  * **Kernel Mode**: 原厂 SPI 驱动 (`SPBTESTTOOL`) + 微软 VHF (Virtual HID Framework)。

## 项目结构

    EGoTouchRev/
    ├── EGoTouchService/           # 主服务程序
    │   ├── source/
    │   │   ├── main.cpp           # 程序入口
    │   │   └── SystemDectector.cpp# 系统事件监测 (Stub)
    │   └── ...
    ├── HimaxChipCore/             # 核心驱动库 (Static Library)
    │   ├── header/
    │   │   ├── HimaxHal.h         # 硬件抽象层 (Win32 Overlapped I/O)
    │   │   └── HimaxChip.h        # 芯片逻辑类 (Chip) 及寄存器定义类
    │   │       ├── ic_operation   # IC 操作寄存器
    │   │       ├── fw_operation   # 固件操作寄存器
    │   │       └── ... (其他 nested classes)
    │   └── ...
    ├── docs/                      # 技术文档
    └── CMakeLists.txt             # 构建脚本

## 构建环境

本项目专为 **Windows on Arm64** 优化。

### 推荐构建方式

1.  **IDE**: Visual Studio Code
2.  **插件**: 安装 **CMake Tools** 和 **Clangd** 插件。
3.  **编译器**: 推荐配置 MSYS2 Clang64 环境或使用 MSVC ARM64 工具链。
4.  **构建**: 使用 VSCode 底部的 CMake 栏直接配置并生成项目即可。

## 功能特性 & 进度 (Current Status)

截止最新更新，项目进度如下：

*   [x] **芯片启动初始化**: 已实现基础启动逻辑（`thp_afe_start`），逻辑接近官方 Debug 工具。
*   [x] **触摸帧读取**: 已实现 Master 和 Slave 设备原始数据流的读取。
*   [x] **寄存器映射**: 完整移植了 Himax IC 的各类寄存器操作类。
*   [ ] **芯片关机逻辑**: `hx_sense_off` 代码已实现，但**未验证**。
*   [ ] **手写笔模式 (240Hz)**: 尚未启动，需逆向特定寄存器指令。
*   [ ] **工作线程实现**: 触摸帧解析线程及系统状态获取线程（屏幕亮灭监听）尚未完成。
*   [ ] **坐标回报**: 目前主要卡在数据解析（见技术文档中的两个方向）。

## 调试与运行

**注意：必须以管理员权限运行。**

程序将尝试打开以下设备句柄：
* `\\.\Global\SPBTESTTOOL_MASTER`
* `\\.\Global\SPBTESTTOOL_SLAVE`

## 免责声明

本项目属于逆向工程研究性质，仅供学习交流。使用本项目替换原厂驱动可能会导致设备触摸功能失效，甚至损坏硬件，请务必在开发前备份原厂驱动服务。

**警告：本项目未经完整测试，可能会损坏硬件，用户需自行承担风险。使用该项目即默认用户自行承担一切后果。**
