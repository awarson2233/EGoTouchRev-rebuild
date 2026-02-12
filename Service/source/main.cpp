#include <iostream>
#include <windows.h>
#include <string>
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

int main() {
    std::cout << "--- EGoTouchService (HimaxChip Lib) ---" << std::endl;
    std::cout << ">>> 必须以管理员权限运行 <<<\n" << std::endl;

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

    chip.thp_afe_start();
    std::cout << "THP AFE initialization completed." << std::endl;

    std::cout << "Exiting..." << std::endl;
    return 0;
}
