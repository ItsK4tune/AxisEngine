#pragma once

#include <editor/i_editor_panel.h>
#include <string>
#include <vector>
#include <entt/entt.hpp>

struct Scene;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Scene Hierarchy"; }
    PanelGroup GetGroup() const override { return PanelGroup::Scene; }

private:
    void DrawEntityNode(Scene& scene, entt::entity entity);
    void DrawComponents(entt::registry& registry, entt::entity entity);
    void CreateNewEntity(Scene& scene, const std::string& sceneName);

private:
    entt::entity m_SelectedEntity = entt::null;
};
