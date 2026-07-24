#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <filesystem>
#include <array>
#include <string>
#include <vector>

struct Scene;

class FileHierarchyPanel : public IEditorPanel
{
public:
    void Initialize() override;
    void OnUpdate(float dt) override
    {
    }
    void OnImGui(Scene& scene) override;
    void DrawContents(Scene& scene);
    std::string GetTitle() const override
    {
        return "Project Files";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Scene;
    }

private:
    void OpenInSystemEditor(const std::filesystem::path& path);

    std::filesystem::path m_CurrentPath;
    std::filesystem::path m_ProjectRoot;
    std::string m_PathInput;

    std::filesystem::path m_SelectedFile;
    std::string m_PreviewContent;
    bool m_PreviewLoaded = false;
    std::array<char, 128> m_AssetName{"NewAsset"};
    int m_NewAssetType = 0;
    bool m_OpenCreateFolder = false;
    bool m_OpenCreateAsset = false;
    bool m_OpenRename = false;
    bool m_OpenDelete = false;
    std::string m_OperationStatus;

    std::vector<std::filesystem::path> m_History;
    size_t m_HistoryIndex = 0;
};
#endif
