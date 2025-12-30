/*
 * @Author: Detach0-0 detach0-0@outlook.com
 * @Date: 2025-12-25 01:21:43
 * @LastEditors: Detach0-0 detach0-0@outlook.com
 * @LastEditTime: 2025-12-25 01:25:05
 * @FilePath: \EGoTouchRev-vsc\HimaxChipCore\source\FrameProcess.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "FrameProcess.h"
#include <array>
#include <vector>
#include <cstdint>

using RawFrame = std::array<uint8_t, 4800>; 

struct Point {
    int index;
    int16_t value;
};

/**
 * @brief 处理原始帧数据，识别手指和手写笔的触摸点
 * @param buffer 原始帧数据缓冲区 (4800 字节)
 * @param touches 输出的触摸点列表
 */
void ProcessFrame(const RawFrame& buffer, std::vector<Point>& touches) {
    const uint8_t* src = buffer.data();
    int total_pixels = 2400; // 40x60

#if HIMAX_HAS_NEON
    // 1. 准备常量
    // 基线 32768 (0x8000)
    const uint16x8_t v_baseline = vdupq_n_u16(32768); 
    // 手指阈值 (+800) -> 33568
    const uint16x8_t v_finger_thresh = vdupq_n_u16(33568);
    // 手写笔阈值 (-5000) -> 27768
    const uint16x8_t v_stylus_thresh = vdupq_n_u16(27768);

    // 2. SIMD 循环 (每次处理 8 个像素 = 16 字节)
    for (int i = 0; i < total_pixels; i += 8) {
        // [Load]: 加载 16 个字节 (8 个像素) 到 128位 寄存器
        // 此时数据还是大端序，且被视为 uint8
        uint8x16_t v_raw_u8 = vld1q_u8(src + i * 2);

        // [Swap]: 关键指令！vrev16q_u8
        // 在每个 16-bit 块内部交换字节 (Big Endian -> Little Endian)
        uint8x16_t v_swapped_u8 = vrev16q_u8(v_raw_u8);

        // [Reinterpret]: 现在字节序对了，把它们看作 uint16
        uint16x8_t v_val = vreinterpretq_u16_u8(v_swapped_u8);

        // --- 此时 v_val 里已经是正确的本机序 16位整数了，可以直接运算 ---

        // [Compare]: 并行比较手指 (> 33568)
        uint16x8_t v_is_finger = vcgtq_u16(v_val, v_finger_thresh);
        
        // [Compare]: 并行比较手写笔 (< 27768)
        uint16x8_t v_is_stylus = vcltq_u16(v_val, v_stylus_thresh);

        // [Check]: 快速检查这 8 个点里有没有感兴趣的数据
        // vmaxvq_u16 会把向量里所有位进行 OR 操作，如果结果为 0，说明这 8 个点全是底噪
        if (vmaxvq_u16(vorrq_u16(v_is_finger, v_is_stylus)) == 0) {
            continue; // 跳过这 8 个点，极其高效
        }

        // [Fallback]: 如果有有效点，才将这 8 个点取出来细看
        // 这里为了代码简单，用了数组回写，实际可以用更高级的位扫描指令
        uint16_t temp_vals[8];
        vst1q_u16(temp_vals, v_val);
        
        for (int j = 0; j < 8; j++) {
            uint16_t val = temp_vals[j];
            // 复查逻辑
            if (val > 33568) {
                touches.push_back({i + j, (int16_t)(val - 32768)}); // 手指
            } else if (val < 27768) {
                touches.push_back({i + j, (int16_t)(val - 32768)}); // 笔
            }
        }
    }
#else
    // Scalar fallback for non-NEON platforms (e.g. x86_64 build environment)
    for (int i = 0; i < total_pixels; ++i) {
        // Data is Big Endian
        // buffer is array of uint8_t
        // Pixel i starts at i*2
        uint16_t high = src[i * 2];
        uint16_t low = src[i * 2 + 1];
        uint16_t val = (high << 8) | low;

        if (val > 33568) {
            touches.push_back({i, (int16_t)(val - 32768)});
        } else if (val < 27768) {
            touches.push_back({i, (int16_t)(val - 32768)});
        }
    }
#endif
}
