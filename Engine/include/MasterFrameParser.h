#pragma once
#include "IFrameProcessor.h"

namespace Engine {

class MasterFrameParser : public IFrameProcessor {
public:
    bool Process(HeatmapFrame& frame) override;
    std::string GetName() const override { return "Master Frame Parser"; }
};

} // namespace Engine
