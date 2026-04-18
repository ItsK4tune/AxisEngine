#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>
#include <filesystem>
#include <vector>

struct Scene;

class FileHierarchyPanel : public IEditorPanel
{
public:
    void Initialize() override;
    void OnUpdate(float dt) override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "File Hierarchy"; }
    PanelGroup GetGroup() const override { return PanelGroup::Scene; }

private:
    void DrawPreview();
    void OpenInSystemEditor(const std::filesystem::path& path);

    std::filesystem::path m_CurrentPath;
    std::string m_PathInput;

    std::filesystem::path m_SelectedFile;
    std::string m_PreviewContent;
    bool m_PreviewLoaded = false;

    // History for Back navigation
    std::vector<std::filesystem::path> m_History;
    size_t m_HistoryIndex = 0;
};
#endif
