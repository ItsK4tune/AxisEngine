#pragma once

#include <editor/i_editor_panel.h>
#include <entt/entity/entity.hpp>
#include <cstdint>
#include <string>
#include <unordered_set>

class NavigationPanel final : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Navigation [Ctrl+Shift+4]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }

private:
    entt::entity m_SelectedProvider = entt::null;
    std::unordered_set<uint32_t> m_SelectedFollowers;
    std::string m_Status;
};
