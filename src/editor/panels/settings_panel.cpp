#include <editor/panels/settings_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

void SettingsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    ImGui::SliderFloat("Time Scale", &m_TimeScale, 0.0f, 5.0f);
    ImGui::Checkbox("VSync", &m_VsyncEnabled);
    ImGui::SliderInt("Frame Limit", &m_FrameLimit, 0, 240);
    ImGui::Checkbox("Pause Engine", &m_IsPaused);

    ImGui::End();
}
#endif
