#include "BaselineSubtraction.h"
#include <arm_neon.h>
#include <cstring>

namespace Engine {

bool BaselineSubtraction::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;

    int16_t* ptr = reinterpret_cast<int16_t*>(frame.heatmapMatrix);

    // 考虑到热力图基底一般是在 0x7FFE 附近浮动
    // 我们可以提取作为变量配置，目前先写死 0x7FFE
    int16x8_t s_base = vdupq_n_s16(0x7FFE); 

    // 可选：死区参数 Deadzone，比如波动在 -15 ~ 15 内的都当做 0 处理
    // int16x8_t dz_upper = vdupq_n_s16(15);
    // int16x8_t dz_lower = vdupq_n_s16(-15);

    // 4800 字节 = 2400 个点，每次处理 8 个点
    for(int i = 0; i < 2400; i += 8) {
        // 加载 8 个 short 数据
        int16x8_t s_raw = vld1q_s16(ptr + i);
        
        // s_raw = s_raw - 0x7FFE
        int16x8_t delta = vsubq_s16(s_raw, s_base);
        
        // TODO: 这里可以用 NEON 指令进行滤波或者死区判断
        // 目前先直接覆盖回去
        vst1q_s16(ptr + i, delta);
    }

    return true;
}

} // namespace Engine
