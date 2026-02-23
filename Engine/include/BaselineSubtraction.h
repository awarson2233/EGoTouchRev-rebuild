#pragma once
#include "IFrameProcessor.h"

namespace Engine {

class BaselineSubtraction : public IFrameProcessor {
public:
    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Baseline Subtraction & Deadzone"; }
};

} // namespace Engine
