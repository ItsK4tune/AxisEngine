#include <editor/panels/resource_browser_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>

void ResourceBrowserPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    auto* rm = ServiceLocator::Instance().Resolve<ResourceManager>();
    if (!rm) {
        ImGui::Text("ResourceManager is not available.");
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("ResourceTabs"))
    {
        if (ImGui::BeginTabItem("Meshes")) {
            auto models = rm->GetLoadedModels();
            if (models.empty()) ImGui::Text("No models loaded.");
            for (const auto& name : models) {
                ImGui::Selectable(name.c_str());
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Textures")) {
            auto textures = rm->GetLoadedTextures();
            if (textures.empty()) ImGui::Text("No textures loaded.");
            for (const auto& name : textures) {
                ImGui::Selectable(name.c_str());
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shaders")) {
            auto shaders = rm->GetLoadedShaders();
            if (shaders.empty()) ImGui::Text("No shaders loaded.");
            for (const auto& name : shaders) {
                ImGui::Selectable(name.c_str());
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
#endif
