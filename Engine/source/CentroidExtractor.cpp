#include "CentroidExtractor.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>

namespace Engine {

std::vector<FingerCenter> TouchSegmenter::analyze_and_segment_blob(
    const std::vector<TouchPoint>& blob, 
    const int16_t global_grid[40][60])
{
    if (blob.empty()) return {};
    
    // 1. Basic Centroid
    float sum_weight = 0.f, sum_x = 0.f, sum_y = 0.f;
    for (const auto& p : blob) {
        sum_weight += p.weight;
        sum_x += p.x * p.weight; 
        sum_y += p.y * p.weight;
    }
    float mean_x = sum_x / sum_weight; 
    float mean_y = sum_y / sum_weight;

    if (blob.size() < 4) return { {mean_x, mean_y, sum_weight} };

    // 2. PCA
    float c_xx = 0.f, c_xy = 0.f, c_yy = 0.f;
    for (const auto& p : blob) {
        float dx = p.x - mean_x; 
        float dy = p.y - mean_y;
        c_xx += p.weight * dx * dx; 
        c_xy += p.weight * dx * dy; 
        c_yy += p.weight * dy * dy;
    }
    c_xx /= sum_weight; c_xy /= sum_weight; c_yy /= sum_weight;

    float b = c_xx + c_yy;
    float c = c_xx * c_yy - c_xy * c_xy;
    float delta = std::max(0.f, b * b - 4 * c);
    float lambda1 = (b + std::sqrt(delta)) / 2.f; 
    float lambda2 = (b - std::sqrt(delta)) / 2.f; 
    if (lambda2 < 0.001f) lambda2 = 0.001f;

    float aspect_ratio = lambda1 / lambda2;

    // Defense 1: Fat Thumb Check
    if (lambda2 > MAX_MINOR_AXIS_VARIANCE) {
        return { {mean_x, mean_y, sum_weight} }; 
    }

    // === 【核心修改：Decision Engine 增加绝对热力阈值干预】 ===
    // 如果长宽比超过阈值，或者总重量极大（对抗肩部融合的斜坡被隐藏），都强行进入切分！
    bool is_merged = (aspect_ratio > ASPECT_RATIO_THRESHOLD) || (sum_weight > HUGE_WEIGHT_THRESHOLD);
    if (!is_merged) {
        return { {mean_x, mean_y, sum_weight} }; 
    }

    // Initialize K-Means centers along the major axis
    float vx = c_xy, vy = lambda1 - c_xx;
    float norm = std::sqrt(vx*vx + vy*vy);
    if (norm > 1e-5f) { vx /= norm; vy /= norm; } else { vx = 1; vy = 0; }
    
    float spread = std::sqrt(lambda1);
    FingerCenter center1 { mean_x + 0.4f * spread * vx, mean_y + 0.4f * spread * vy, 0.f };
    FingerCenter center2 { mean_x - 0.4f * spread * vx, mean_y - 0.4f * spread * vy, 0.f };

    // K-Means Iteration (4 times is enough for convergence)
    for (int iter = 0; iter < 4; ++iter) {
        float sum_w1 = 0.f, sum_x1 = 0.f, sum_y1 = 0.f;
        float sum_w2 = 0.f, sum_x2 = 0.f, sum_y2 = 0.f;
        
        for (const auto& p : blob) {
            float d1 = (p.x - center1.x)*(p.x - center1.x) + (p.y - center1.y)*(p.y - center1.y);
            float d2 = (p.x - center2.x)*(p.x - center2.x) + (p.y - center2.y)*(p.y - center2.y);
            if (d1 < d2) {
                sum_w1 += p.weight; sum_x1 += p.x * p.weight; sum_y1 += p.y * p.weight;
            } else {
                sum_w2 += p.weight; sum_x2 += p.x * p.weight; sum_y2 += p.y * p.weight;
            }
        }
        
        if (sum_w1 > 0) { center1.x = sum_x1 / sum_w1; center1.y = sum_y1 / sum_w1; center1.total_weight = sum_w1; }
        if (sum_w2 > 0) { center2.x = sum_x2 / sum_w2; center2.y = sum_y2 / sum_w2; center2.total_weight = sum_w2; }
    }

    // Defense 2: Physical Bone-Distance Check
    float final_dist = std::sqrt(std::pow(center1.x - center2.x, 2) + std::pow(center1.y - center2.y, 2));
    if (final_dist < MIN_PHYSICAL_DISTANCE) {
        return { {mean_x, mean_y, sum_weight} }; 
    }

    // Defense 3: Valley Profile Check
    int mid_x = std::clamp((int)std::round((center1.x + center2.x) / 2.0f), 0, 59);
    int mid_y = std::clamp((int)std::round((center1.y + center2.y) / 2.0f), 0, 39);
    
    int cy1 = std::clamp((int)std::round(center1.y), 0, 39);
    int cx1 = std::clamp((int)std::round(center1.x), 0, 59);
    int cy2 = std::clamp((int)std::round(center2.y), 0, 39);
    int cx2 = std::clamp((int)std::round(center2.x), 0, 59);

    int c1_val = global_grid[cy1][cx1];
    int c2_val = global_grid[cy2][cx2];
    int mid_val = global_grid[mid_y][mid_x];

    if (mid_val >= std::min(c1_val, c2_val)) {
        // === 【同步修改：这里也应使用常量而非写死 8000】 ===
        if (sum_weight < HUGE_WEIGHT_THRESHOLD) {
            return { {mean_x, mean_y, sum_weight} };
        }
    }

    return {center1, center2};
}

// ---------------------------------------------------------
// 后续的 CentroidExtractor::Process 与原版完全保持一致，无需修改。
// 因为你的 BFS 和 8连通域算法写得非常标准且高效！
// ...

CentroidExtractor::CentroidExtractor() {}
CentroidExtractor::~CentroidExtractor() {}

bool CentroidExtractor::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;

    frame.contacts.clear();
    const int numRows = 40;
    const int numCols = 60;
    
    int touchId = 1;

    // 1. Connected Component Labeling (BFS) to gather Blobs
    std::vector<bool> visited(numRows * numCols, false);
    std::vector<std::vector<TouchPoint>> blobs;

    for (int y = 0; y < numRows; ++y) {
        for (int x = 0; x < numCols; ++x) {
            float val = static_cast<float>(frame.heatmapMatrix[y][x]);
            if (!visited[y * numCols + x] && val >= m_peakThreshold) {
                // Found a new unvisited pixel above threshold, start BFS
                std::vector<TouchPoint> currentBlob;
                std::vector<std::pair<int, int>> queue;
                queue.push_back({x, y});
                visited[y * numCols + x] = true;
                
                size_t head = 0;
                while (head < queue.size()) {
                    auto p = queue[head++];
                    int cx = p.first;
                    int cy = p.second;
                    currentBlob.push_back({static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(frame.heatmapMatrix[cy][cx])});
                    
                    // 8-connectivity to boldly merge diagonally touching pixels into the same blob
                    static const int dxs[] = {-1, 0, 1, -1, 1, -1, 0, 1};
                    static const int dys[] = {-1, -1, -1, 0, 0, 1, 1, 1};
                    for (int i = 0; i < 8; ++i) {
                        int nx = cx + dxs[i];
                        int ny = cy + dys[i];
                        if (nx >= 0 && nx < numCols && ny >= 0 && ny < numRows) {
                            int nIdx = ny * numCols + nx;
                            if (!visited[nIdx] && frame.heatmapMatrix[ny][nx] >= m_peakThreshold) {
                                visited[nIdx] = true;
                                queue.push_back({nx, ny});
                            }
                        }
                    }
                }
                if (!currentBlob.empty()) {
                    blobs.push_back(std::move(currentBlob));
                }
            }
        }
    }

    // 2. Segment each blob
    for (const auto& blob : blobs) {
        std::vector<FingerCenter> centers = TouchSegmenter::analyze_and_segment_blob(blob, frame.heatmapMatrix);
        
        for (const auto& c : centers) {
            TouchContact tc;
            tc.id = touchId++;
            // CalculateGaussianParaboloid handles subpixel resolution. 
            // In PCA we already have subpixel mean_x and mean_y natively, but we can refine it if strictly requested.
            // Since PCA handles this incredibly well on its own, we just output it directly.
            float outY = c.y;
            float outX = m_algorithm == 1 ? CalculateGaussianParaboloid(frame, static_cast<int>(std::round(c.x)), static_cast<int>(std::round(c.y)), outY) : c.x;
            
            tc.x = m_algorithm == 1 ? outX : c.x;
            tc.y = m_algorithm == 1 ? outY : c.y;
            tc.state = 0; 
            tc.area = c.total_weight;  // Approximated Area
            frame.contacts.push_back(tc);
        }
    }

    return true;
}

float CentroidExtractor::CalculateGaussianParaboloid(const HeatmapFrame& frame, int cx, int cy, float& outY) const {
    // 1D Parabola Fitting for X and Y Separately around Peak
    auto GetV = [&](int dx, int dy) -> float {
        int nx = cx + dx;
        int ny = cy + dy;
        if (nx >= 0 && nx < 60 && ny >= 0 && ny < 40) {
            return std::max<float>(0.0f, static_cast<float>(frame.heatmapMatrix[ny][nx]));
        }
        return 0.0f;
    };

    float vCenter = GetV(0, 0);
    float vLeft   = GetV(-1, 0);
    float vRight  = GetV(1, 0);
    float vTop    = GetV(0, -1);
    float vBottom = GetV(0, 1);

    float denomX = 2.0f * (vLeft - 2.0f * vCenter + vRight);
    float dx = 0.0f;
    if (std::abs(denomX) > 0.001f) {
        dx = (vLeft - vRight) / denomX;
    }

    float denomY = 2.0f * (vTop - 2.0f * vCenter + vBottom);
    float dy = 0.0f;
    if (std::abs(denomY) > 0.001f) {
        dy = (vTop - vBottom) / denomY;
    }

    dx = std::clamp(dx, -0.5f, 0.5f);
    dy = std::clamp(dy, -0.5f, 0.5f);

    outY = cy + dy;
    return cx + dx;
}

void CentroidExtractor::DrawConfigUI() {
    ImGui::TextWrapped("PCA-KMeans Algorithm for Smart Centroid Extraction:");
    ImGui::RadioButton("Native PCA Weight Centroid", &m_algorithm, 0);
    ImGui::RadioButton("2D Paraboloid Refinement", &m_algorithm, 1);
    
    ImGui::SliderInt("Peak Detection Threshold", &m_peakThreshold, 50, 2000);
}

} // namespace Engine
