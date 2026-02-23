#include "DiagnosticUI.h"
#include "HimaxChip.h"
#include "imgui.h"
#include "Logger.h"


namespace App {

DiagnosticUI::DiagnosticUI(Coordinator* coordinator) : m_coordinator(coordinator) {
}

DiagnosticUI::~DiagnosticUI() {
}

void DiagnosticUI::Render() {
    // 拉取最新的数据
    if (m_autoRefresh && m_coordinator) {
        m_coordinator->GetLatestFrame(m_currentFrame);
    }

    // 绘制控制面板和热力图窗口
    DrawControlPanel();
    DrawHeatmap();
}

void DiagnosticUI::DrawControlPanel() {
    ImGui::Begin("Device Control Panel");
    
    if (m_coordinator) {
        Himax::Chip* chip = m_coordinator->GetDevice();
        if (chip) {
            auto connState = chip->GetConnectionState();
            bool connected = (connState == Himax::ConnectionState::Connected);

            ImGui::Text("Connection Status: %s", connected ? "Connected" : "Unconnected");
            
            if (ImGui::Button("Chip::Init")) {
                if (!connected) {
                    LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "Chip Init User Action");
                    [[maybe_unused]] auto _r = chip->Init();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Chip::Deinit")) { // Renamed Disconnect to Deinit for consistency
                if (connected) {
                    LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "Chip Deinit User Action");
                    [[maybe_unused]] auto _r = chip->Deinit();
                }
            }
            
            ImGui::Separator();
            
            bool loopActive = m_coordinator->IsAcquisitionActive();
            if (!connected) ImGui::BeginDisabled();
            if (ImGui::Button(loopActive ? "Stop Reading Loop" : "Start Reading Loop")) {
                m_coordinator->SetAcquisitionActive(!loopActive);
                LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "{} Reading Loop User Action", 
                    loopActive ? "Stop" : "Start");
            }
            if (!connected) ImGui::EndDisabled();

            ImGui::Separator();
            
            if (!connected) ImGui::BeginDisabled();
            

            if (ImGui::Button("Enter Idle")) {
                LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "Enter Idle User Action");
                [[maybe_unused]] auto _r = chip->thp_afe_enter_idle(0);
            }
            ImGui::SameLine();
            if (ImGui::Button("Exit Idle")) {
                LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "Exit Idle User Action");
                [[maybe_unused]] auto _r = chip->thp_afe_force_exit_idle();
            }
            if (ImGui::Button("Start Calibration")) {
                LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "Start Calibration User Action");
                [[maybe_unused]] auto _r = chip->thp_afe_start_calibration(0);
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Status")) {
                LOG_INFO("App", "DiagnosticUI::DrawControlPanel", "UI", "Clear Status User Action");
                [[maybe_unused]] auto _r = chip->thp_afe_clear_status(1);
            }
            
            if (!connected) ImGui::EndDisabled();
        }
    }
    if (m_coordinator) {
        ImGui::Separator();
        ImGui::Text("Engine Pipeline Config");
        for (const auto& processor : m_coordinator->GetPipeline().GetProcessors()) {
            bool enabled = processor->IsEnabled();
            if (ImGui::Checkbox(processor->GetName().c_str(), &enabled)) {
                processor->SetEnabled(enabled);
            }
        }
    }

    ImGui::Checkbox("Auto-refresh Heatmap", &m_autoRefresh);
    ImGui::Checkbox("Fullscreen Heatmap", &m_fullscreen);
    ImGui::SliderInt("Heatmap Scale", &m_heatmapScale, 1, 30);
    
    ImGui::End();
}

void DiagnosticUI::DrawHeatmap() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (m_fullscreen) {
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    ImGui::Begin("Raw Heatmap Visualization (40x60)", nullptr, window_flags);
    
    // In multi-viewport mode, if the window is outside, GetContentRegionAvail might refer to its own OS window
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    ImVec2 canvas_p = ImGui::GetCursorScreenPos();

    const int rows = 40; // TX 
    const int cols = 60; // RX 

    // 计算动态缩放比例
    float cell_w, cell_h;
    if (m_fullscreen || canvas_sz.x > 800) { // If fullscreen or manually expanded
        cell_w = canvas_sz.x / cols;
        cell_h = canvas_sz.y / rows;
    } else {
        cell_w = (float)m_heatmapScale;
        cell_h = (float)m_heatmapScale;
    }

    if (!m_fullscreen) {
        ImGui::Text("Timestamp: %llu | Cell Size: %.1fx%.1f", (unsigned long long)m_currentFrame.timestamp, cell_w, cell_h);
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            // Mirror Matrix: Left becomes Right, Top becomes Bottom
            int16_t val = m_currentFrame.heatmapMatrix[rows - 1 - y][cols - 1 - x];
            
            // 值映射 (假设正常触摸引起的电容变化(delta)范围在 0 ~ 2000 左右)
            // 你可以根据实际数值调整这个比例
            float normalized = std::clamp(val / 1000.0f, 0.0f, 1.0f);
            
            // 冷暖色调映射 (深蓝 -> 绿 -> 黄 -> 红)
            ImVec4 colorVec;
            if (normalized < 0.25f) colorVec = ImVec4(0, 0, normalized * 4.0f, 1.0f);
            else if (normalized < 0.5f) colorVec = ImVec4(0, (normalized - 0.25f) * 4.0f, 1.0f - (normalized - 0.25f) * 4.0f, 1.0f);
            else if (normalized < 0.75f) colorVec = ImVec4((normalized - 0.5f) * 4.0f, 1.0f, 0, 1.0f);
            else colorVec = ImVec4(1.0f, 1.0f - (normalized - 0.75f) * 4.0f, 0, 1.0f);
            
            ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);
            
            ImVec2 p_min = ImVec2(canvas_p.x + x * cell_w, canvas_p.y + y * cell_h);
            ImVec2 p_max = ImVec2(p_min.x + cell_w, p_min.y + cell_h);
            
            draw_list->AddRectFilled(p_min, p_max, color);
            
            // 画边线 (全屏模式下为了 1:1 视觉效果可以考虑减淡或者移除边线)
            draw_list->AddRect(p_min, p_max, IM_COL32(50, 50, 50, m_fullscreen ? 50 : 255));
        }
    }
    
    if (!m_fullscreen) {
        // 根据格子占用空间，手动往下推 ImGui 的 Cursor
        ImGui::Dummy(ImVec2(cols * cell_w, rows * cell_h));
    }
    
    ImGui::End();
}

} // namespace App
