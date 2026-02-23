#pragma once
#include <vector>
#include <cstdint>
#include <array>

namespace Engine {

// 触摸点结构体 (用于 Stage 2 连通域计算)
struct TouchContact {
    int id;
    float x;
    float y;
    int state; // 0=Down, 1=Update, 2=Up
    int area;  // 连通域大小或强度
};

// 整个管线中流转的帧结构体
struct HeatmapFrame {
    // 原始下发的 5063 字节数据 （通常在解析后可以释放掉或清空）
    std::vector<uint8_t> rawData;
    
    // 40 x 60 的热力图矩阵, 数据类型 int16_t (便于基线减去后支持负数的死区操作)
    int16_t heatmapMatrix[40][60];
    
    // 从 heatmap 中解析出来的触控点列表
    std::vector<TouchContact> contacts;

    // 时间戳或其他元数据
    uint64_t timestamp;

    HeatmapFrame() : timestamp(0) {
        // 初始化矩阵全0
        for (int i=0; i<40; ++i) {
            for (int j=0; j<60; ++j) {
                heatmapMatrix[i][j] = 0;
            }
        }
    }
};

} // namespace Engine
