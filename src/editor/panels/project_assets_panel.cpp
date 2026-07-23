#include <editor/panels/project_assets_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <resource/logic/resource_manager.h>
#include <imgui.h>
#include <algorithm>

void ProjectAssetsPanel::Initialize()
{
    m_ProjectFiles.Initialize();
    m_LoadedResources.Initialize();
}

void ProjectAssetsPanel::Shutdown()
{
    m_LoadedResources.Shutdown();
    m_ProjectFiles.Shutdown();
}

void ProjectAssetsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    if (ImGui::BeginTabBar("ProjectAssetsTabs"))
    {
        if (ImGui::BeginTabItem("Project"))
        {
            m_ProjectFiles.DrawContents(scene);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Loaded Resources"))
        {
            m_LoadedResources.DrawContents(scene);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Import / Dependencies"))
        {
            auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
            if (resources)
            {
                auto types = resources->GetRegisteredLoaderTypes();
                std::sort(types.begin(), types.end());
                if (!types.empty())
                {
                    m_ImportType = std::clamp(m_ImportType, 0, static_cast<int>(types.size() - 1));
                    ImGui::InputTextWithHint("##ImportPath", "Asset path", m_ImportPath.data(), m_ImportPath.size());
                    ImGui::SameLine();
                    if (ImGui::BeginCombo("##ImportType", types[m_ImportType].c_str()))
                    {
                        for (size_t index = 0; index < types.size(); ++index)
                            if (ImGui::Selectable(types[index].c_str(), static_cast<int>(index) == m_ImportType))
                                m_ImportType = static_cast<int>(index);
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Import") && m_ImportPath[0] != '\0')
                        m_ImportStatus = resources->LoadUnified(types[m_ImportType], m_ImportPath.data())
                                             ? "Asset imported."
                                             : "Importer rejected the asset.";
                    ImGui::TextDisabled("Watched shader/texture assets reimport automatically after file changes.");
                }
                if (!m_ImportStatus.empty())
                    ImGui::TextWrapped("%s", m_ImportStatus.c_str());

                ImGui::SeparatorText("Resource dependencies");
                for (const auto& definition : resources->GetResourceDefinitions())
                {
                    ImGui::PushID((definition.type + ":" + definition.name).c_str());
                    if (ImGui::TreeNode("resource", "%s (%s)", definition.name.c_str(),
                                        definition.type.c_str()))
                    {
                        for (const auto& [property, value] : definition.properties)
                            ImGui::BulletText("%s -> %s", property.c_str(), value.c_str());
                        if (ImGui::Button("Reimport"))
                            m_ImportStatus = resources->ReimportResource(definition.name)
                                                 ? "Resource reimported: " + definition.name
                                                 : "Reimport failed: " + definition.name;
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

#endif
