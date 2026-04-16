#include <editor/panels/scene_hierarchy_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <scene/logic/scene.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/unit/core_components.h>

void SceneHierarchyPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto& registry = scene.registry;
    auto view = registry.view<InfoComponent>();

    for (auto entity : view)
    {
        uint32_t entityId = static_cast<uint32_t>(entity);
        std::string name = "Entity " + std::to_string(entityId);
        std::string tag = "";

        if (registry.all_of<InfoComponent>(entity))
        {
            auto& info = registry.get<InfoComponent>(entity);
            tag = info.tag;
            name = info.name;
        }

        DrawEntityNode(scene, entityId, name, tag, false, {});
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Scene& scene, uint32_t entityId, const std::string& name,
                                         const std::string& tag, bool hasChildren,
                                         const std::vector<uint32_t>& children)
{
    ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entityId) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
    flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entityId, flags, "%s", name.c_str());
    if (ImGui::IsItemClicked())
    {
        m_SelectedEntity = entityId;
    }

    if (opened && hasChildren)
    {
        // Recursively draw children
        ImGui::TreePop();
    }
}
#endif
