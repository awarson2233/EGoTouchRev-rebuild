#include "SpatialSharpenFilter.h"
#include "imgui.h"
#include <vector>
#include <algorithm>

namespace Engine {

bool SpatialSharpenFilter::Process(HeatmapFrame& frame) {
    if (!m_enabled || m_strength <= 0.0f) return true;

    const int numRows = 40;
    const int numCols = 60;

    // Use a temporary buffer since we need neighborhood pixels
    std::vector<int16_t> tempBuffer(numRows * numCols, 0);

    // Laplacian kernel: 
    //  0 -1  0
    // -1  4 -1
    //  0 -1  0
    for (int y = 1; y < numRows - 1; ++y) {
        for (int x = 1; x < numCols - 1; ++x) {
            int16_t top = frame.heatmapMatrix[y - 1][x];
            int16_t bot = frame.heatmapMatrix[y + 1][x];
            int16_t left = frame.heatmapMatrix[y][x - 1];
            int16_t right = frame.heatmapMatrix[y][x + 1];
            int16_t center = frame.heatmapMatrix[y][x];

            // Laplacian computes the 2nd derivative (rate of change of the slope)
            int32_t laplace = (4 * center) - top - bot - left - right;
            
            // Unsharp Masking: Original + Strength * Laplacian
            // This aggressively steepens edges and deepens valleys between close flat peaks
            int32_t sharpened = center + static_cast<int32_t>(m_strength * laplace);

            // Clamp correctly to prevent negative artifacts or integer overflows
            sharpened = std::clamp(sharpened, 0, 4095);
            
            tempBuffer[y * numCols + x] = static_cast<int16_t>(sharpened);
        }
    }

    // Write back to the frame (edges stay 0 or unsharpened since kernel can't reach them)
    for (int y = 1; y < numRows - 1; ++y) {
        for (int x = 1; x < numCols - 1; ++x) {
            frame.heatmapMatrix[y][x] = tempBuffer[y * numCols + x];
        }
    }

    return true;
}

void SpatialSharpenFilter::DrawConfigUI() {
    ImGui::TextWrapped("Digs deep valleys between extremely close flat peaks to aid Watershed Centroid parsing.");
    ImGui::SliderFloat("Sharpening Strength", &m_strength, 0.1f, 5.0f, "%.1fx");
}

} // namespace Engine
