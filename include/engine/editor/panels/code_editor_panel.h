#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>
#include <vector>
#include <filesystem>

struct Scene;

class CodeEditorPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "File Hierarchy"; }

private:
    void DrawDirectory(const std::filesystem::path& dirPath);
    void LoadFilePreview(const std::filesystem::path& filePath);
    void OpenNativeEditor(const std::filesystem::path& filePath);

    std::string   m_CurrentPath = "l:/C++/AxisEngine";
    std::string   m_PreviewContent;
    std::string   m_SelectedFile;
};
#endif
