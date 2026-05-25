#pragma once

#include <editor/i_editor_panel.h>
#include <entt/entt.hpp>
#include <string>
#include <vector>

struct Scene;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    static entt::entity s_SelectedEntity;

    static void SetSelectedEntity(entt::entity entity);
    static void RequestFocus(entt::entity entity);

    void Initialize() override
    {
    }
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "Scene Hierarchy [Ctrl+1]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Scene;
    }

private:
    void DrawEntityNode(Scene& scene, entt::entity entity);
    void DrawComponents(entt::registry& registry, entt::entity entity);
    void CreateNewEntity(Scene& scene, const std::string& sceneName);
    void DuplicateEntity(Scene& scene, entt::entity entity);

    char m_SearchFilter[256] = "";
    bool m_CtrlDPressed = false;
};
