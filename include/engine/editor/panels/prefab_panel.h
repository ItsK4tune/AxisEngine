#pragma once

#include <editor/i_editor_panel.h>
#include <entt/entity/entity.hpp>
#include <array>
#include <string>
#include <vector>

class PrefabPanel final : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Prefabs [Ctrl+Shift+7]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }

private:
    bool WriteSelection(Scene& scene, entt::entity source, bool replaceSelection);
    bool CreateInstance(Scene& scene, const std::string& path);
    bool ApplyInstance(Scene& scene);
    bool RevertInstance(Scene& scene);
    std::array<char, 260> m_Path{"assets/prefabs/new_prefab.axs"};
    std::array<char, 4096> m_OverrideText{};
    entt::entity m_OverrideEntity = entt::null;
    entt::entity m_CreateSource = entt::null;
    std::vector<std::string> m_DiscoveredPrefabs;
    bool m_ReplaceSelection = true;
    bool m_AllowOverwrite = false;
    std::string m_Status;
};
