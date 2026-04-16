#include <editor/panels/tools_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

void ToolsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Wireframe Mode", &m_Wireframe);
        ImGui::Checkbox("No Textures", &m_NoTexture);
        ImGui::Checkbox("Enable Shadows", &m_Shadows);
        ImGui::Checkbox("Enable Skybox", &m_Skybox);
    }
    
    if (ImGui::CollapsingHeader("Debug Vis", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Physics Debug", &m_PhysicsDebug);
        ImGui::Checkbox("Entity Names", &m_EntityNames);
        ImGui::Checkbox("Gizmos", &m_Gizmos);
        ImGui::Checkbox("Light Gizmos", &m_LightGizmos);
        ImGui::Checkbox("UI Enabled", &m_UIEnabled);
    }

    ImGui::End();
}
#endif
