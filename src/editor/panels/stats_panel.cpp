#include <editor/panels/stats_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

void StatsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    ImGui::Text("FPS: %.1f", m_Fps);
    ImGui::Text("Frame Time: %.3f ms", m_FrameTime);
    ImGui::Separator();
    ImGui::Text("Engine Status: OK");

    ImGui::End();
}
#endif
