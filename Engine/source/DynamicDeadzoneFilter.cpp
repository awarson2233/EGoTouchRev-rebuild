#include "DynamicDeadzoneFilter.h"
#include "imgui.h"
#include <algorithm>

#if defined(_M_ARM64)
#include <arm_neon.h>
#endif

namespace Engine {

bool DynamicDeadzoneFilter::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;

    const int numPixels = 40 * 60;
    int16_t* frameData = &frame.heatmapMatrix[0][0];

    // 1. 寻找全屏范围内的最大正波峰 (Global Max)
    int16_t globalMax = 0;
#if defined(_M_ARM64)
    int16x8_t vMax = vdupq_n_s16(0);
    int i = 0;
    for (; i <= numPixels - 8; i += 8) {
        int16x8_t vData = vld1q_s16(&frameData[i]);
        vMax = vmaxq_s16(vMax, vData);
    }
    globalMax = vmaxvq_s16(vMax);
    for (; i < numPixels; ++i) {
        globalMax = std::max(globalMax, frameData[i]);
    }
#else
    for (int i = 0; i < numPixels; ++i) {
        globalMax = std::max(globalMax, frameData[i]);
    }
#endif

    if (globalMax <= 0) return true;

    // 设置全局动态截断水位线
    int16_t shrinkVal = static_cast<int16_t>((static_cast<int32_t>(globalMax) * m_shrinkPercent) / 100);

    // 2. 将全屏信号统一向下推顶 shrinkVal，实现全局水位软切除
    if (shrinkVal > 0) {
#if defined(_M_ARM64)
        int16x8_t vShrink = vdupq_n_s16(shrinkVal);
        int16x8_t vZero = vdupq_n_s16(0);
        int i = 0;
        for (; i <= numPixels - 8; i += 8) {
            int16x8_t vData = vld1q_s16(&frameData[i]);
            // 结果 = max(0, vData - shrinkVal)
            int16x8_t vRes = vmaxq_s16(vZero, vsubq_s16(vData, vShrink));
            vst1q_s16(&frameData[i], vRes);
        }
        for (; i < numPixels; ++i) {
            frameData[i] = std::max<int16_t>(0, frameData[i] - shrinkVal);
        }
#else
        for (int i = 0; i < numPixels; ++i) {
            frameData[i] = std::max<int16_t>(0, frameData[i] - shrinkVal);
        }
#endif
    }
    return true;
}

void DynamicDeadzoneFilter::DrawConfigUI() {
    ImGui::TextWrapped("Reduces global noise by shrinking the entire matrix based on the global peak signal.");
    ImGui::SliderInt("Global Peak Shrink (%)", &m_shrinkPercent, 0, 100);
}

} // namespace Engine
