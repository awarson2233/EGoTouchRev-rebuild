#pragma once

#include "FramePipeline.h"
#include <string>

namespace Engine {

// 新一代统一前处理滤波器：保留信号真实波峰形态
// 1. IIR 时域稳定 (滤除屏幕全局闪烁)
// 2. 线性底噪切除水平面 (Water-level Clipping，避免分段撕裂)
class SignalConditioningFilter : public IFrameProcessor {
public:
    SignalConditioningFilter();
    ~SignalConditioningFilter() override;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Signal Conditioning (IIR + Clip)"; }

    void DrawConfigUI() override;

private:
    int16_t m_historyData[40 * 60];
    bool m_hasHistory;

    // Adjustable Parameters
    int m_alpha = 700;       // IIR Weight for current frame (0-1000)
    int m_noiseFloor = 80;   // The global threshold below which signals are considered 0
};

} // namespace Engine
