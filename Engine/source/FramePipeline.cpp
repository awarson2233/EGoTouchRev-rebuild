#include "FramePipeline.h"
#include <algorithm>

namespace Engine {

void FramePipeline::AddProcessor(std::unique_ptr<IFrameProcessor> processor) {
    if (processor) {
        m_processors.push_back(std::move(processor));
    }
}

void FramePipeline::RemoveProcessor(const std::string& name) {
    m_processors.erase(
        std::remove_if(m_processors.begin(), m_processors.end(),
            [&name](const std::unique_ptr<IFrameProcessor>& p) {
                return p->GetName() == name;
            }),
        m_processors.end());
}

void FramePipeline::MoveProcessorUp(size_t index) {
    if (index > 0 && index < m_processors.size()) {
        std::swap(m_processors[index], m_processors[index - 1]);
    }
}

void FramePipeline::MoveProcessorDown(size_t index) {
    if (index >= 0 && index + 1 < m_processors.size()) {
        std::swap(m_processors[index], m_processors[index + 1]);
    }
}

bool FramePipeline::Execute(HeatmapFrame& frame) {
    for (auto& processor : m_processors) {
        if (!processor->Process(frame)) {
            // If any processor returns false, the frame is dropped
            return false;
        }
    }
    return true;
}

const std::vector<std::unique_ptr<IFrameProcessor>>& FramePipeline::GetProcessors() const {
    return m_processors;
}

} // namespace Engine
