#include <editor/panels/code_editor_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

#include <fstream>
#include <sstream>
#include <windows.h>

void CodeEditorPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    // Split into 2 panes: Tree view (left) and Preview (right)
    ImGui::Columns(2, "FileBrowserColumns", true);
    if (ImGui::GetColumnWidth() < 200.0f) ImGui::SetColumnWidth(0, 200.0f);

    // Left pane: Directory tree
    ImGui::BeginChild("FileTree");
    DrawDirectory(std::filesystem::path(m_CurrentPath));
    ImGui::EndChild();

    ImGui::NextColumn();

    // Right pane: Preview
    ImGui::BeginChild("FilePreview");
    if (!m_SelectedFile.empty()) {
        ImGui::Text("Previewing: %s", std::filesystem::path(m_SelectedFile).filename().string().c_str());
        ImGui::Separator();
        ImGui::TextUnformatted(m_PreviewContent.c_str());
    } else {
        ImGui::Text("Select a file to preview.");
    }
    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::End();
}

void CodeEditorPanel::DrawDirectory(const std::filesystem::path& dirPath)
{
    if (!std::filesystem::exists(dirPath)) return;

    for (const auto& entry : std::filesystem::directory_iterator(dirPath))
    {
        const auto& path = entry.path();
        std::string filename = path.filename().string();
        
        // Skip hidden files/dirs
        if (filename.empty() || filename[0] == '.') continue;
        
        // Skip build folder to avoid massive lag
        if (filename == "build" || filename == "bin" || filename == ".git") continue;

        if (entry.is_directory())
        {
            if (ImGui::TreeNodeEx(filename.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                DrawDirectory(path);
                ImGui::TreePop();
            }
        }
        else
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (m_SelectedFile == path.string()) flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::TreeNodeEx(filename.c_str(), flags);
            if (ImGui::IsItemClicked(0)) // Left click preview
            {
                m_SelectedFile = path.string();
                LoadFilePreview(path);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) // Double click to open OS editor
            {
                OpenNativeEditor(path);
            }
        }
    }
}

void CodeEditorPanel::LoadFilePreview(const std::filesystem::path& filePath)
{
    m_PreviewContent.clear();
    std::ifstream file(filePath);
    if (file.is_open())
    {
        // Read up to 200 lines for preview
        std::string line;
        int linesRead = 0;
        while (std::getline(file, line) && linesRead < 200) {
            m_PreviewContent += line + "\n";
            linesRead++;
        }
        if (!file.eof()) {
            m_PreviewContent += "\n... (File truncated for preview)";
        }
    }
    else {
        m_PreviewContent = "Could not open file.";
    }
}

void CodeEditorPanel::OpenNativeEditor(const std::filesystem::path& filePath)
{
    std::string pathStr = std::filesystem::absolute(filePath).string();
    ShellExecuteA(NULL, "open", pathStr.c_str(), NULL, NULL, SW_SHOW);
}
#endif
