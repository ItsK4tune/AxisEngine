#include <editor/panels/resource_browser_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

void ResourceBrowserPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    if (ImGui::BeginTabBar("ResourceTabs"))
    {
        if (ImGui::BeginTabItem("Meshes")) {
            ImGui::Text("No meshes loaded.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Textures")) {
            ImGui::Text("No textures loaded.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Materials")) {
            ImGui::Text("No materials loaded.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
#endif
