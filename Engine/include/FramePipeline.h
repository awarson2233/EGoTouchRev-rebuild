#pragma once
#include "IFrameProcessor.h"
#include <vector>
#include <memory>

namespace Engine {

class FramePipeline {
public:
    void AddProcessor(std::unique_ptr<IFrameProcessor> processor);
    void RemoveProcessor(const std::string& name);
    void MoveProcessorUp(size_t index);
    void MoveProcessorDown(size_t index);
    
    // Execute the processors in sequence
    // Returns false if the frame is completely dropped by a processor
    bool Execute(HeatmapFrame& frame); 

    // Retrieve all processors to allow GUI to toggle them
    const std::vector<std::unique_ptr<IFrameProcessor>>& GetProcessors() const;

private:
     std::vector<std::unique_ptr<IFrameProcessor>> m_processors;
};

} // namespace Engine
