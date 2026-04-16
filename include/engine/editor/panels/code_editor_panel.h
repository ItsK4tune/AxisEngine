#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>
#include <vector>

struct Scene;

class CodeEditorPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Code Editor"; }

private:
    void BrowseForFile();
    void LoadFile(const std::string& path);
    void SaveFile();

    std::string              m_CurrentPath;
    std::vector<char>        m_Buffer;
    bool                     m_UnsavedChanges = false;
    bool                     m_BrowseOpen     = false;
    static constexpr size_t  k_BufferSize     = 1024 * 64;
};
#endif
