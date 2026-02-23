#include "Coordinator.h"
#include "Logger.h"
#include <chrono>

namespace App {

// --- 设备路径 ---
const std::wstring DEVICE_PATH_INTERRUPT = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring DEVICE_PATH_MASTER = L"\\\\.\\Global\\SPBTESTTOOL_MASTER";
const std::wstring DEVICE_PATH_SLAVE = L"\\\\.\\Global\\SPBTESTTOOL_SLAVE";

Coordinator::Coordinator() {
    LOG_INFO("App", "Coordinator::Coordinator", "Unconnected", "Initializing Himax Device Instance (Unconnected)...");
    // 初始化 Hardware 层对象，此时只分配资源，不拉起 I2C 通信
    m_device = std::make_unique<Himax::Chip>(DEVICE_PATH_MASTER, DEVICE_PATH_SLAVE, DEVICE_PATH_INTERRUPT);

    // Initialise Engine Pipeline
    m_pipeline.AddProcessor(std::make_unique<Engine::MasterFrameParser>());
    m_pipeline.AddProcessor(std::make_unique<Engine::BaselineSubtraction>());
}

Coordinator::~Coordinator() {
    Stop();
}

bool Coordinator::Start() {
    if (m_running.exchange(true)) return false; // Already running

    LOG_INFO("App", "Coordinator::Start", "Unknown", "Starting background threads...");
    
    // 启动 Himax AFE (假设它已经在构造里或外面调了，或者在这里调)
    // 也可以留给 GUI 去手动点击 "Start AFE"
    
    m_acquisitionThread = std::thread(&Coordinator::AcquisitionThreadFunc, this);
    m_processingThread = std::thread(&Coordinator::ProcessingThreadFunc, this);
    m_systemStateThread = std::thread(&Coordinator::SystemStateThreadFunc, this);

    return true;
}

void Coordinator::Stop() {
    if (!m_running.exchange(false)) return;

    LOG_INFO("App", "Coordinator::Stop", "Unknown", "Stopping background threads...");

    if (m_acquisitionThread.joinable()) m_acquisitionThread.join();
    if (m_processingThread.joinable()) m_processingThread.join();
    if (m_systemStateThread.joinable()) m_systemStateThread.join();
}

bool Coordinator::GetLatestFrame(Engine::HeatmapFrame& outFrame) {
    std::lock_guard<std::mutex> lock(m_latestFrameMutex);
    outFrame = m_latestFrame;
    return true;
}

void Coordinator::AcquisitionThreadFunc() {
    LOG_INFO("App", "Coordinator::AcquisitionThreadFunc", "Unknown", "Acquisition Thread started.");
    
    while (m_running) {
        if (m_device->GetConnectionState() != Himax::ConnectionState::Connected || !m_isAcquiring.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (auto res = m_device->GetFrame(); !res) {
            // Handle error, maybe logging is enough for now as Device does it
            continue;
        }
        
        // 提取采集的数据到帧对象
        Engine::HeatmapFrame frame;
        
        // m_device->back_data is std::array<uint8_t, ...> inside the HAL. 
        // We copy it out for processing. We take the 5063 master bytes (+339 slave)
        frame.rawData.assign(m_device->back_data.begin(), m_device->back_data.end());

        // Push Raw data into Processing Thread
        m_frameBuffer.Push(frame);

        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // Polling Interval
    }
    LOG_INFO("App", "Coordinator::AcquisitionThreadFunc", "Unknown", "Acquisition Thread stopped.");
}

void Coordinator::ProcessingThreadFunc() {
    LOG_INFO("App", "Coordinator::ProcessingThreadFunc", "Unknown", "Processing Thread started.");
    while (m_running) {
        Engine::HeatmapFrame frame;
        // 阻塞等待采集线程 push 原始帧
        if (m_frameBuffer.WaitForData(frame, std::chrono::milliseconds(100))) {
            
            // Execute the pipeline (MasterFrameParser -> BaselineSubtraction -> ...)
            if (m_pipeline.Execute(frame)) {

                // 如果处理成功, 写回给 GUI 
                {
                    std::lock_guard<std::mutex> lock(m_latestFrameMutex);
                    m_latestFrame = frame;
                }

                // TODO: (Stage 3) 交给 Host::VhfInjector 发送 HID Report
            }
        }
    }
    LOG_INFO("App", "Coordinator::ProcessingThreadFunc", "Unknown", "Processing Thread stopped.");
}

void Coordinator::SystemStateThreadFunc() {
    LOG_INFO("App", "Coordinator::SystemStateThreadFunc", "Unknown", "SystemState Thread started.");
    // TODO: (Stage 3) 使用 Host/SystemMonitor 监听亮屏/息屏，并调用 m_device->thp_afe_enter_idle()
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    LOG_INFO("App", "Coordinator::SystemStateThreadFunc", "Unknown", "SystemState Thread stopped.");
}

} // namespace App
