#pragma once
#include "EngineTypes.h"
#include <string>

namespace Engine {

class IFrameProcessor {
public:
    virtual ~IFrameProcessor() = default;

    // Process frame. Return false to Drop the frame
    virtual bool Process(HeatmapFrame& frame) = 0;

    // Get module name
    virtual std::string GetName() const = 0;

    // Get/Set enable status
    virtual bool IsEnabled() const { return m_enabled; }
    virtual void SetEnabled(bool enabled) { m_enabled = enabled; }

protected:
    bool m_enabled = true;
};

} // namespace Engine
