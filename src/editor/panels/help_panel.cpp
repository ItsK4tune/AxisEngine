#include <editor/panels/help_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

void HelpPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    ImGui::Text("========== DEBUG CONTROLS ==========");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("VISUAL TOGGLES", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("F6        : Toggle Wireframe");
        ImGui::Text("Shift+F6  : Toggle Skybox");
        ImGui::Text("F7        : Toggle No-Texture Mode");
        ImGui::Text("Shift+F7  : Toggle Shadows");
    }

    if (ImGui::CollapsingHeader("SYSTEM TOGGLES", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("F8        : Toggle Physics Debug");
        ImGui::Text("Shift+F8  : Toggle Audio Debug");
        ImGui::Text("F9        : Toggle UI System");
        ImGui::Text("Shift+F9  : Toggle Particle Debug");
    }

    if (ImGui::CollapsingHeader("GAME CONTROL", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("F11       : Pause/Resume Game");
        ImGui::Text("Shift+F11 : Toggle Debug Camera (Free Cam)");
        ImGui::Text("F12       : Cycle Time Scale (0.25x -> 2x)");
        ImGui::Text("Shift+F12 : Cycle Cursor Mode");
    }

    ImGui::End();
}
#endif
