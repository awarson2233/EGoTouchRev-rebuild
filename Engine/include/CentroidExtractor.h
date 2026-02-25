#pragma once
#include "IFrameProcessor.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Engine {

struct TouchPoint {
    float x;
    float y;
    float weight;
};

struct FingerCenter {
    float x;
    float y;
    float total_weight;
};

class TouchSegmenter {
public:
    // --- 经过 DVR 回放数据验证的最佳动态阈值 ---
    static constexpr float ASPECT_RATIO_THRESHOLD = 1.9f; 
    static constexpr float MAX_MINOR_AXIS_VARIANCE = 5.0f; // 从4.0放宽，允许拖拽时的“彗尾”变形
    static constexpr float MIN_PHYSICAL_DISTANCE = 1.8f;   // 从2.5缩紧，允许双指在滑动时互相挤压靠得更近
    static constexpr float HUGE_WEIGHT_THRESHOLD = 7000.f; // 新增：绝对重压阈值（对抗肩部融合）

    static std::vector<FingerCenter> analyze_and_segment_blob(
        const std::vector<TouchPoint>& blob, 
        const int16_t global_grid[40][60]);
};

class CentroidExtractor : public IFrameProcessor {
public:
    CentroidExtractor();
    ~CentroidExtractor() override;

    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "PCA-KMeans Centroid Extractor"; }

    void DrawConfigUI() override;

private:
    float CalculateGaussianParaboloid(const HeatmapFrame& frame, int cx, int cy, float& outY) const;

    int m_algorithm = 1; // 0 for Native PCA, 1 for Gaussian Paraboloid
    int m_peakThreshold = 80; // 建议默认底噪下调到 80，提高边缘响应
    float m_minPeakDist = 4.0f;
};

} // namespace Engine