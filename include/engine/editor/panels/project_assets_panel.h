#pragma once

#include <editor/i_editor_panel.h>
#include <editor/panels/file_hierarchy_panel.h>
#include <editor/panels/resource_browser_panel.h>
#include <array>

class ProjectAssetsPanel final : public IEditorPanel
{
public:
    void Initialize() override;
    void Shutdown() override;
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Project / Assets [Ctrl+2]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Scene; }

private:
    FileHierarchyPanel m_ProjectFiles;
    ResourceBrowserPanel m_LoadedResources;
    std::array<char, 260> m_ImportPath{};
    int m_ImportType = 0;
    std::string m_ImportStatus;
};
