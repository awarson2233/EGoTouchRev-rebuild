#pragma once

#include "HimaxChip.h"
#include "RingBuffer.h"
#include <thread>
#include <atomic>
#include <memory>
#include <vector>

// Engine Includes
#include "EngineTypes.h"
#include "FramePipeline.h"
#include "MasterFrameParser.h"
#include "BaselineSubtraction.h"

namespace App {

class Coordinator {
public:
    Coordinator();
    ~Coordinator();

    bool Start();
    void Stop();
    
    // GUI 交互接口，用于手动控制芯片
    Himax::Chip* GetDevice() { return m_device.get(); }
    
    // 注入供 GUI 使用的最新热力图引用
    bool GetLatestFrame(Engine::HeatmapFrame& outFrame);

    // 获取数据处理管线，用于 GUI 动态配置
    Engine::FramePipeline& GetPipeline() { return m_pipeline; }

    // 数据采集循环控制
    void SetAcquisitionActive(bool active) { m_isAcquiring.store(active); }
    bool IsAcquisitionActive() const { return m_isAcquiring.load(); }

private:
    void AcquisitionThreadFunc();
    void ProcessingThreadFunc();
    void SystemStateThreadFunc();

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isAcquiring{false};
    
    // Modules
    std::unique_ptr<Himax::Chip> m_device;
    
    // Threads
    std::thread m_acquisitionThread;
    std::thread m_processingThread;
    std::thread m_systemStateThread;

    // Engine Pipeline
    Engine::FramePipeline m_pipeline;

    // Data flow
    RingBuffer<Engine::HeatmapFrame, 16> m_frameBuffer;
    
    // GUI needs the latest frame synchronously
    std::mutex m_latestFrameMutex;
    Engine::HeatmapFrame m_latestFrame;
};

} // namespace App
