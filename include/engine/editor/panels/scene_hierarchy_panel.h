#pragma once

#include <editor/i_editor_panel.h>
#include <entt/entt.hpp>
#include <array>
#include <span>
#include <string>
#include <vector>

struct Scene;

class SceneHierarchyPanel : public IEditorPanel
{
public:
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
    void SetSelectedEntity(entt::entity entity);
    void ToggleSelectedEntity(entt::entity entity);
    bool IsSelected(entt::entity entity) const;
    void RequestFocus(entt::entity entity);
    void DrawEntityNode(Scene& scene, entt::entity entity);
    void CreateNewEntity(Scene& scene, const std::string& sceneName);
    void DuplicateEntity(Scene& scene, entt::entity entity);

    char m_SearchFilter[256] = "";
    bool m_CtrlDPressed = false;
    bool m_DeletePressed = false;
};
