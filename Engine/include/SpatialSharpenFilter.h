#pragma once
#include "IFrameProcessor.h"
#include <string>

namespace Engine {

class SpatialSharpenFilter : public IFrameProcessor {
public:
    SpatialSharpenFilter() = default;
    ~SpatialSharpenFilter() override = default;

    bool Process(HeatmapFrame& frame) override;
    
    std::string GetName() const override { return "Spatial Sharpen Filter"; }
    
    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enabled) override { m_enabled = enabled; }
    
    void DrawConfigUI() override;

private:
    bool m_enabled = false; // Default off, let the user toggle when needed
    float m_strength = 1.0f; // Sharpening factor
};

} // namespace Engine
