#include "MasterFrameParser.h"
#include <arm_neon.h>
#include <cstring>
#include <stdexcept>

namespace Engine {

bool MasterFrameParser::Process(HeatmapFrame& frame) {
    if (!m_enabled) return true;

    // 根据需求：Master 帧原始长度 5063 字节
    // 跳过前 7 字节，取中间 4800 字节 (40 TX * 60 RX * 2 Byte)
    if (frame.rawData.size() < 5063) {
        return true; 
    }

    const uint8_t* raw_ptr = frame.rawData.data() + 7;
    int16_t* heat_ptr = reinterpret_cast<int16_t*>(frame.heatmapMatrix); 

    // 4800 字节 = 2400 个 uint16_t 数据
    // 每次处理 8 个数据 (128位寄存器)，共 300 次循环
    // NEON Intrinsics 展开
    for (int i = 0; i < 2400; i += 8) {
        // 从无对齐的 uint8_t 内存流加载 8 个 uint16_t 数据 -> 128 bit 寄存器
        uint16x8_t raw_vals = vld1q_u16(reinterpret_cast<const uint16_t*>(raw_ptr + i * 2));
        
        // 直接存入 HeatmapMatrix 的 int16_t 数组中 (强制转化)
        vst1q_s16(heat_ptr + i, vreinterpretq_s16_u16(raw_vals));
    }

    return true;
}

} // namespace Engine
