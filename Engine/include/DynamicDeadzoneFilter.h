#pragma once

#include "FramePipeline.h"
#include <string>

namespace Engine {

// 动态行死区滤波器
// 针对电容屏特有的“行共模噪声 (Row Common-mode Noise)”
// 取每行当前的最大波峰的百分比作为动态水平面，切除该水平面以下的起伏。
class DynamicDeadzoneFilter : public IFrameProcessor {
public:
    DynamicDeadzoneFilter() = default;
    ~DynamicDeadzoneFilter() override = default;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Dynamic Row Deadzone"; }

    void DrawConfigUI() override;

private:
    int m_shrinkPercent = 20; // 默认取行最高峰的 20% 作为噪声截断线
};

} // namespace Engine
