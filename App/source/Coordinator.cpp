#include "Coordinator.h"
#include "Logger.h"
#include "DynamicDeadzoneFilter.h"
#include "SignalConditioningFilter.h"
#include "GaussianFilter.h"
#include "SpatialSharpenFilter.h"
#include "CentroidExtractor.h"
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
    m_pipeline.AddProcessor(std::make_unique<Engine::DynamicDeadzoneFilter>());
    m_pipeline.AddProcessor(std::make_unique<Engine::SignalConditioningFilter>());
    m_pipeline.AddProcessor(std::make_unique<Engine::GaussianFilter>());
    // Unsharp masking filter to separate highly merged fingers *before* extraction
    m_pipeline.AddProcessor(std::make_unique<Engine::SpatialSharpenFilter>());
    m_pipeline.AddProcessor(std::make_unique<Engine::CentroidExtractor>());
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

                // Push to DVR buffer (automatically overwrites old frames)
                m_dvrBuffer.PushOverwriting(frame);

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

void Coordinator::TriggerDVRExport() {
    auto snapshot = m_dvrBuffer.GetSnapshot();
    if (snapshot.empty()) {
        LOG_WARN("App", "Coordinator::TriggerDVRExport", "Unknown", "DVR buffer is empty, nothing to export.");
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm time_info;
    localtime_s(&time_info, &time_t_now);

    char filename[128];
    sprintf_s(filename, "dvr_backtrack_%04d%02d%02d_%02d%02d%02d.csv",
              time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
              time_info.tm_hour, time_info.tm_min, time_info.tm_sec);

    FILE* fp = nullptr;
    fopen_s(&fp, filename, "w");
    if (!fp) {
        LOG_ERROR("App", "Coordinator::TriggerDVRExport", "Unknown", "Failed to create DVR export file: %s", filename);
        return;
    }

    LOG_INFO("App", "Coordinator::TriggerDVRExport", "Unknown", "Exporting %zu frames to %s...", snapshot.size(), filename);

    for (size_t i = 0; i < snapshot.size(); ++i) {
        const auto& f = snapshot[i];
        fprintf(fp, "--- Frame [%zu] --- TS: %llu\n", i, f.timestamp);
        
        // Heatmap
        for (int y = 0; y < 40; ++y) {
            for (int x = 0; x < 60; ++x) {
                fprintf(fp, "%d%s", f.heatmapMatrix[y][x], (x == 59 ? "" : ","));
            }
            fprintf(fp, "\n");
        }

        // Contacts
        fprintf(fp, "Contacts: %zu\n", f.contacts.size());
        for (const auto& c : f.contacts) {
            fprintf(fp, "ID:%d, X:%.3f, Y:%.3f, State:%d, Area:%d\n", 
                    c.id, c.x, c.y, c.state, c.area);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    LOG_INFO("App", "Coordinator::TriggerDVRExport", "Unknown", "DVR Export Complete: %s", filename);
}

} // namespace App
