#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "HimaxChip.h"

// --- 设备路径 ---
const std::wstring DEVICE_PATH_INTERRUPT = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring DEVICE_PATH_MASTER = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring DEVICE_PATH_SLAVE = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";

// --- 数据大小 ---
#define HEADER_SIZE 339  // Slave 数据包大小
#define BODY_SIZE   5063 // Master 数据包大小 (热力图)

// 用于线程同步
std::mutex mtx;
std::condition_variable cv;
bool g_DataReady = false;
bool g_Shutdown = false;

/**
 * @brief 线程 A：中断监听器
 * 持续等待硬件中断发生。
 */
void InterruptListenerThread(Himax::Chip* chip) {
    std::cout << "[Thread A] Interrupt listener started. Waiting for touch..." << std::endl;

    while (!g_Shutdown) {
        // 阻塞等待中断
        // WaitInterrupt 内部封装了 IOCTL_WAIT_ON_INTERRUPT 和 WaitForSingleObject
        if (chip->WaitInterrupt()) {
            // 中断发生！通知数据读取线程
            {
                std::lock_guard<std::mutex> lock(mtx);
                g_DataReady = true;
            }
            cv.notify_one(); // 唤醒 Thread B
        }
        else {
            // 等待出错或超时
            // 可以添加适当的延时避免 CPU 占用过高（如果 WaitInterrupt 立即返回失败）
            std::cerr << "[Thread A] WaitInterrupt failed or timed out." << std::endl;
            Sleep(100); 
        }
    }

    std::cout << "[Thread A] Interrupt listener shutting down." << std::endl;
}

/**
 * @brief 线程 B：数据读取器
 * 在收到中断信号后，读取 Header 和 Body 数据。
 */
void DataReaderThread(Himax::Chip* chip) {
    std::vector<uint8_t> headerBuffer(HEADER_SIZE);
    std::vector<uint8_t> bodyBuffer(BODY_SIZE);

    std::cout << "[Thread B] Data reader started. Waiting for signal..." << std::endl;

    while (!g_Shutdown) {
        // 1. 等待来自 Thread A 的信号
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return g_DataReady || g_Shutdown; });

        if (g_Shutdown) break;

        // 2. 读取 Header (Slave)
        if (chip->GetSlaveData(headerBuffer)) {
            std::cout << "[Thread B] Read Header (" << headerBuffer.size() << " bytes) OK." << std::endl;
            // 在这里添加代码来解析 headerBuffer
        }
        else {
            std::cerr << "[Thread B] Error: Read Header failed." << std::endl;
        }

        // 3. 读取 Body (Master)
        if (chip->GetMasterData(bodyBuffer)) {
            std::cout << "[Thread B] Read Body (" << bodyBuffer.size() << " bytes) OK." << std::endl;
            // 在这里添加代码来解析 bodyBuffer (这就是热力图数据)
            // 例如，打印前 4 个字节：
            if (bodyBuffer.size() >= 4) {
                printf("  > Body[0-3]: %02X %02X %02X %02X\n", bodyBuffer[0], bodyBuffer[1], bodyBuffer[2], bodyBuffer[3]);
            }
        }
        else {
            std::cerr << "[Thread B] Error: Read Body failed." << std::endl;
        }

        // 4. 重置标志位
        g_DataReady = false;
        lock.unlock();
    }
    std::cout << "[Thread B] Data reader shutting down." << std::endl;
}

// 优雅地处理 Ctrl+C 退出
BOOL WINAPI ConsoleHandler(DWORD CEvent) {
    if (CEvent == CTRL_C_EVENT) {
        std::cout << "Ctrl+C detected, shutting down..." << std::endl;
        {
            std::lock_guard<std::mutex> lock(mtx);
            g_Shutdown = true;
        }
        cv.notify_all(); // 唤醒所有等待的线程
    }
    return TRUE;
}

int main() {
    std::cout << "--- EGoTouchService (HimaxChip Lib) ---" << std::endl;
    std::cout << ">>> 必须以管理员权限运行 <<<\n" << std::endl;

    if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE) {
        std::cerr << "Error: Could not set control handler." << std::endl;
        return 1;
    }

    // 1. 初始化 Chip 对象
    // 注意：Chip 构造函数会尝试打开设备句柄
    Himax::Chip chip(DEVICE_PATH_MASTER, DEVICE_PATH_SLAVE, DEVICE_PATH_INTERRUPT);

    // 检查设备是否就绪
    if (!chip.IsReady(Himax::DeviceType::Master) || 
        !chip.IsReady(Himax::DeviceType::Slave) || 
        !chip.IsReady(Himax::DeviceType::Interrupt)) {
        std::cerr << "Error: Failed to open one or more devices." << std::endl;
        return 1;
    }

    std::cout << "All device handles opened successfully." << std::endl;

    // 2. 硬件初始化 (可选，取决于是否需要重新初始化硬件)
    // 如果此程序是作为服务运行，通常需要初始化硬件。
    // 如果只是测试工具且硬件已由其他服务初始化，则可跳过。
    // 这里我们调用 thp_afe_start 进行初始化流程。
    std::cout << "Starting THP AFE initialization..." << std::endl;
    chip.thp_afe_start();
    std::cout << "THP AFE initialization completed." << std::endl;

    // 3. 启动线程
    std::thread threadA(InterruptListenerThread, &chip);
    std::thread threadB(DataReaderThread, &chip);

    // 等待线程结束
    threadA.join();
    threadB.join();

    std::cout << "Exiting..." << std::endl;
    return 0;
}
