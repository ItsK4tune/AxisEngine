#include <editor/panels/inspector_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <editor/editor_selection.h>
#include <editor/editor_system.h>
#include <scene/logic/scene.h>
#include <imgui.h>

void InspectorPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    EditorSystem::BeginPanelTransactionOnContentClick(scene, "Inspector edit");
    auto& selection = ServiceLocator::Instance().Require<EditorSelection>();
    const entt::entity primary = selection.GetPrimary();
    if (primary != entt::null && scene.IsValid(primary))
    {
        if (selection.GetAll().size() > 1)
            ImGui::TextDisabled("%zu entities selected; editing primary.", selection.GetAll().size());
        m_Inspector.Draw(scene.GetRegistry(), primary);
    }
    else
    {
        ImGui::TextDisabled("No entity selected");
    }
    ImGui::End();
}

#endif
