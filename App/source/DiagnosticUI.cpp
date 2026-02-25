#include "DiagnosticUI.h"
#include "HimaxChip.h"
#include "imgui.h"
#include "Logger.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>

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
    DrawCoordinateTable();
    DrawMasterSuffixTable();
    DrawSlaveSuffixTable();
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
    
    ImGui::Separator();
    if (ImGui::Button("Export Frame to CSV")) {
        ExportCurrentFrameToCSV();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export DVR (Last 120 Frames)")) {
        if (m_coordinator) {
            m_coordinator->TriggerDVRExport();
        }
    }
    
    if (m_coordinator) {
        ImGui::Separator();
        ImGui::Text("Engine Pipeline Config");
        const auto& processors = m_coordinator->GetPipeline().GetProcessors();
        for (size_t i = 0; i < processors.size(); ++i) {
            auto& processor = processors[i];
            ImGui::PushID(static_cast<int>(i));
            
            bool enabled = processor->IsEnabled();
            if (ImGui::Checkbox(processor->GetName().c_str(), &enabled)) {
                processor->SetEnabled(enabled);
            }
            
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
            if (ImGui::Button("^") && i > 0) {
                m_coordinator->GetPipeline().MoveProcessorUp(i);
                ImGui::PopID();
                break; // Stop rendering this frame to avoid iterator issues
            }
            ImGui::SameLine();
            if (ImGui::Button("v") && i < processors.size() - 1) {
                m_coordinator->GetPipeline().MoveProcessorDown(i);
                ImGui::PopID();
                break; // Stop rendering this frame
            }
            
            if (enabled) {
                ImGui::Indent();
                processor->DrawConfigUI();
                ImGui::Unindent();
            }
            ImGui::PopID();
        }
    }

    ImGui::Checkbox("Auto-refresh Heatmap", &m_autoRefresh);
    ImGui::Checkbox("Fullscreen Heatmap", &m_fullscreen);
    ImGui::SliderInt("Heatmap Scale", &m_heatmapScale, 1, 30);
    ImGui::SliderFloat("Color Max Range", &m_colorRange, 100.0f, 10000.0f, "%.0f");
    
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
            
            // 值映射 (使用用户界面可调的最大量程)
            float normalized = std::clamp(val / m_colorRange, 0.0f, 1.0f);
            
            // 使用更平滑的多段插值 (如 Jet 或 Turbo 极简近似映射) 替代简单的 4 段线性插值
            // 这将最大化利用 ImU32 的 8位单通道精度 (共约 1677 万色)
            ImVec4 colorVec;
            if (normalized <= 0.0f) {
                // Background Base Noise -> Absolute Black
                colorVec = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            } else {
                float r = 0.0f, g = 0.0f, b = 0.0f;
                // Jet Colormap Approximation
                // normalized: (0, 1]
                float fourValue = 4.0f * normalized;
                r = std::clamp(std::min(fourValue - 1.5f, -fourValue + 4.5f), 0.0f, 1.0f);
                g = std::clamp(std::min(fourValue - 0.5f, -fourValue + 3.5f), 0.0f, 1.0f);
                b = std::clamp(std::min(fourValue + 0.5f, -fourValue + 2.5f), 0.0f, 1.0f);
                colorVec = ImVec4(r, g, b, 1.0f);
            }
            
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

void DiagnosticUI::DrawCoordinateTable() {
    ImGui::Begin("Parsed Coordinates");

    if (m_currentFrame.contacts.empty()) {
        ImGui::Text("No touches detected.");
    } else {
        if (ImGui::BeginTable("ContactsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("X (Sub-pixel)", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Y (Sub-pixel)", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Peak Intensity", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            for (const auto& contact : m_currentFrame.contacts) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", contact.id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.4f", contact.x);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.4f", contact.y);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", contact.area);
            }
            ImGui::EndTable();
        }
    }

    ImGui::End();
}

void DiagnosticUI::DrawMasterSuffixTable() {
    ImGui::Begin("Master Frame Suffix (128 words)");
    if (m_currentFrame.rawData.size() >= 5063) {
        if (ImGui::BeginTable("MasterSuffixTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
            const uint8_t* ptr = m_currentFrame.rawData.data() + 4807;
            for (int i = 0; i < 128; ++i) {
                if (i % 8 == 0) ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(i % 8);
                uint16_t val = static_cast<uint16_t>(ptr[i * 2] | (ptr[i * 2 + 1] << 8));
                ImGui::Text("[%03d]: %04X (%5d)", i, val, val);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::Text("Insufficient frame data length.");
    }
    ImGui::End();
}

void DiagnosticUI::DrawSlaveSuffixTable() {
    ImGui::Begin("Slave Frame Suffix (166 words)");
    if (m_currentFrame.rawData.size() >= 5402) { // 5063 + 339 = 5402
        if (ImGui::BeginTable("SlaveSuffixTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
            const uint8_t* ptr = m_currentFrame.rawData.data() + 5070; // 5063 + 7
            for (int i = 0; i < 166; ++i) {
                if (i % 8 == 0) ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(i % 8);
                uint16_t val = static_cast<uint16_t>(ptr[i * 2] | (ptr[i * 2 + 1] << 8));
                ImGui::Text("[%03d]: %04X (%5d)", i, val, val);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::Text("Insufficient slave overlay data length.");
    }
    ImGui::End();
}

void DiagnosticUI::ExportCurrentFrameToCSV() {
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_now = std::localtime(&time_now);
    
    std::ostringstream filename;
    filename << "heatmap_" 
             << std::put_time(tm_now, "%Y%m%d_%H%M%S") << "_" 
             << (m_currentFrame.timestamp % 1000) 
             << ".csv";

    std::ofstream out(filename.str());
    if (!out.is_open()) {
        LOG_INFO("App", "DiagnosticUI::ExportCurrentFrameToCSV", "Error", "Failed to open file for writing.");
        return;
    }

    out << "--- EGoTouch Frame Export ---\n";
    out << "Timestamp: " << m_currentFrame.timestamp << "\n\n";

    out << "--- Heatmap (40 rows x 60 cols) ---\n";
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 60; ++x) {
            out << m_currentFrame.heatmapMatrix[y][x];
            if (x < 59) out << ",";
        }
        out << "\n";
    }

    out << "\n--- Master Frame Suffix (128 words) ---\n";
    if (m_currentFrame.rawData.size() >= 5063) {
        const uint8_t* ptr = m_currentFrame.rawData.data() + 4807;
        for (int i = 0; i < 128; ++i) {
            uint16_t val = static_cast<uint16_t>(ptr[i * 2] | (ptr[i * 2 + 1] << 8));
            out << val;
            if (i < 127) {
                out << (((i + 1) % 16 == 0) ? "\n" : ",");
            }
        }
        out << "\n";
    } else {
        out << "Data unavailable\n";
    }

    out << "\n--- Slave Frame Suffix (166 words) ---\n";
    if (m_currentFrame.rawData.size() >= 5402) {
        const uint8_t* ptr = m_currentFrame.rawData.data() + 5070;
        for (int i = 0; i < 166; ++i) {
            uint16_t val = static_cast<uint16_t>(ptr[i * 2] | (ptr[i * 2 + 1] << 8));
            out << val;
            if (i < 165) {
                out << (((i + 1) % 16 == 0) ? "\n" : ",");
            }
        }
        out << "\n";
    } else {
        out << "Data unavailable\n";
    }

    out.close();
    LOG_INFO("App", "DiagnosticUI::ExportCurrentFrameToCSV", "UI", "Frame exported to {}", filename.str());
}

} // namespace App
