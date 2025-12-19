EGoTouch-Rev
======================

**EGoTouch-Rev** 是针对 **Huawei Matebook E Go (Windows on Arm64)** 设备触摸屏驱动服务的高性能重构项目。

本项目的目标是替代原厂效率较低的 `HuaweiThpService.exe`，通过原生 C++23 直接与底层 SPI 驱动通信，实现更低延迟的触摸响应和更稳定的热力图（Heatmap）处理，彻底消除“鬼影”触控问题。
核心硬件与架构
-------

* **目标设备**: Huawei Matebook E Go (2022性能版)

* **Touch IC**: Himax HX83121A (SPI Interface)

* **架构设计**:
  
  * **User Mode**: `EGoTouchService` (本服务) - 负责 SPI 通信、协议解析、热力图算法。
  
  * **Kernel Mode**: 华为原厂 SPI 驱动 (`SPBTESTTOOL`) + 微软 VHF (Virtual HID Framework)。
  
  * **通信协议**: 基于 Himax F2/F3 (Master) & F4/F5 (Slave) 指令集的自定义 SPI 协议。

项目结构
----

    EGoTouchRev/
    ├── EGoTouchService/       # 主服务程序
    │   ├── source/main.cpp    # 线程管理 (中断监听/数据读取)
    │   └── ...
    ├── HimaxChipCore/         # 核心驱动库 (Static Library)
    │   ├── header/HimaxHal.h  # 硬件抽象层 (Win32 Overlapped I/O)
    │   ├── header/HimaxChip.h # 寄存器定义与逻辑封装
    │   └── ...
    ├── CMakeLists.txt         # 构建脚本
    └── ...

构建环境
----

本项目专为 **Windows on Arm64** 优化，推荐使用 MSYS2 Clang64 环境进行构建。

### 依赖工具

* Visual Studio Code (配合 Clangd 插件)

* MSYS2 (MinGW-w64 Clang-aarch64)

* CMake >= 3.10

### 编译步骤

    # 1. 用AI完成vscode配置

    # 2. 编译

功能特性 (Current Status)
---------------------

* [x] **Modern C++23**: 采用最新标准编写，利用强类型和模块化设计。

* [x] **HAL 层封装**: 实现了 `HimaxHal` 类，封装 `DeviceIoControl` 和 `WriteFile`，支持协议指纹自动识别。

* [x] **多线程架构**: 分离中断监听 (Thread A) 与数据读取 (Thread B)，确保高频采样不阻塞。

* [x] **寄存器映射**: 完整移植了 Himax IC 的寄存器定义 (`ic_operation`, `fw_operation` 等)。

* [x] **基础通信**: 已打通 Master/Slave 设备的握手与 ID 读取。

待办事项 (ToDo / Roadmap)
---------------------

当前版本处于 **Pre-Alpha** 阶段，以下关键功能尚需完善：

* [ ] **初始化流程完善 (High Priority)**:
  
  * [ ] **缓冲区擦除 (Buffer Erasure)**: 在 `init_buffers_and_register` 阶段，需向 `0x10007550` 写入全 0 数据以确保存储区重置。
  
  * [ ] **看门狗配置 (Watchdog Config)**: 实现 `InitOnIcOperation` 中定义的看门狗复位与配置逻辑 (`adr_wdg_ctrl`)，防止芯片在长时运行后死锁。

* [ ] **热力图解析**: 将读取到的 Body 数据解析为坐标点（优化原厂算法）。

* [ ] **VHF 注入**: 对接 Virtual HID Framework，将解析后的触控点上报给操作系统。

* [ ] **计算加速**: 利用 Neon 指令集和 NPU 加速数据解算。

调试与运行
-----

**注意：可能需要以管理员权限运行。**
    # 在构建目录下
    ./EGoTouchService.exe

程序将尝试打开以下设备句柄：

* `\\.\Global\SPBTESTTOOL_MASTER`

* `\\.\Global\SPBTESTTOOL_SLAVE`

免责声明
----

本项目属于逆向工程研究性质，仅供学习交流。使用本项目替换原厂驱动可能会导致设备触摸功能失效，甚至损坏硬件，请务必在开发前备份原厂驱动服务。

**警告：本项目未经完整测试，可能会损坏硬件，请用户自行承担风险。使用该项目即默认用户自行承担一切后果。**
