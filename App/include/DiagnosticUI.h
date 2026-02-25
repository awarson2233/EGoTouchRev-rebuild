#pragma once

#include "Coordinator.h"

namespace App {

class DiagnosticUI {
public:
    // 传入 Coordinator 指针以调用芯片功能并获取最新热力图序列
    explicit DiagnosticUI(Coordinator* coordinator);
    ~DiagnosticUI();

    // 在主循环中每帧调用此函数以绘制 ImGui 界面
    void Render();

private:
    void DrawControlPanel();
    void DrawHeatmap();
    void DrawCoordinateTable();
    void DrawMasterSuffixTable();
    void DrawSlaveSuffixTable();

    void ExportCurrentFrameToCSV();

private:
    Coordinator* m_coordinator;
    
    // 缓存的最新的热力图数据
    Engine::HeatmapFrame m_currentFrame;
    
    // GUI 内部状态
    bool m_autoRefresh = true;
    bool m_fullscreen = false;
    int m_heatmapScale = 10;
    float m_colorRange = 1000.0f;
};

} // namespace App
