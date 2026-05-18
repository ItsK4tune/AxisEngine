#include <editor/panels/file_hierarchy_panel.h>

#ifdef ENABLE_EDITOR
#include <core/logic/logger.h>
#include <imgui.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#endif

void FileHierarchyPanel::Initialize()
{
    m_CurrentPath = std::filesystem::current_path();
    m_PathInput = m_CurrentPath.string();
    m_History.push_back(m_CurrentPath);
    m_HistoryIndex = 0;
}

static void NavigateTo(std::filesystem::path& current, std::string& input, std::vector<std::filesystem::path>& history,
                       size_t& histIdx, const std::filesystem::path& target)
{
    // Trim forward history if branching
    if (histIdx + 1 < history.size())
        history.erase(history.begin() + histIdx + 1, history.end());
    current = target;
    input = target.string();
    history.push_back(target);
    histIdx = history.size() - 1;
}

void FileHierarchyPanel::OpenInSystemEditor(const std::filesystem::path& path)
{
#ifdef _WIN32
    ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

void FileHierarchyPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    // --- Nav Bar ---
    bool canBack = m_HistoryIndex > 0;
    bool canFwd = m_HistoryIndex + 1 < m_History.size();

    if (!canBack)
        ImGui::BeginDisabled();
    if (ImGui::Button("< Back"))
    {
        m_HistoryIndex--;
        m_CurrentPath = m_History[m_HistoryIndex];
        m_PathInput = m_CurrentPath.string();
        m_SelectedFile.clear();
        m_PreviewContent.clear();
        m_PreviewLoaded = false;
    }
    if (!canBack)
        ImGui::EndDisabled();

    ImGui::SameLine();

    if (!canFwd)
        ImGui::BeginDisabled();
    if (ImGui::Button("Fwd >"))
    {
        m_HistoryIndex++;
        m_CurrentPath = m_History[m_HistoryIndex];
        m_PathInput = m_CurrentPath.string();
        m_SelectedFile.clear();
        m_PreviewContent.clear();
        m_PreviewLoaded = false;
    }
    if (!canFwd)
        ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Up"))
    {
        if (m_CurrentPath.has_parent_path())
        {
            NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, m_CurrentPath.parent_path());
            m_SelectedFile.clear();
            m_PreviewContent.clear();
            m_PreviewLoaded = false;
        }
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(-60.0f);
    char pathBuf[512];
    strncpy(pathBuf, m_PathInput.c_str(), sizeof(pathBuf) - 1);
    pathBuf[sizeof(pathBuf) - 1] = '\0';
    if (ImGui::InputText("##Path", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        m_PathInput = pathBuf;
        std::filesystem::path p(m_PathInput);
        if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
        {
            NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, p);
            m_SelectedFile.clear();
            m_PreviewContent.clear();
            m_PreviewLoaded = false;
        }
        else
        {
            m_PathInput = m_CurrentPath.string();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Go"))
    {
        std::filesystem::path p(m_PathInput);
        if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
        {
            NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, p);
            m_SelectedFile.clear();
            m_PreviewContent.clear();
            m_PreviewLoaded = false;
        }
        else
        {
            m_PathInput = m_CurrentPath.string();
        }
    }

    ImGui::Separator();

    // --- Split: File List (left) | Preview (right) ---
    float availW = ImGui::GetContentRegionAvail().x;
    float listW = m_PreviewLoaded ? availW * 0.5f : availW;

    ImGui::BeginChild("FileList", ImVec2(listW, 0), false);
    try
    {
        // Sort: dirs first, then files
        std::vector<std::filesystem::directory_entry> entries;
        for (auto& e : std::filesystem::directory_iterator(m_CurrentPath)) entries.push_back(e);
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            if (a.is_directory() != b.is_directory())
                return a.is_directory() > b.is_directory();
            return a.path().filename() < b.path().filename();
        });

        for (const auto& entry : entries)
        {
            const auto& path = entry.path();
            std::string filename = path.filename().string();
            bool isDir = entry.is_directory();
            bool isSelected = (path == m_SelectedFile);

            if (isDir)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.65f, 0.1f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

            std::string label = (isDir ? "[D] " : "[F] ") + filename;
            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (isDir)
                {
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, path);
                        m_SelectedFile.clear();
                        m_PreviewContent.clear();
                        m_PreviewLoaded = false;
                    }
                }
                else
                {
                    // Single click: load preview
                    if (m_SelectedFile != path)
                    {
                        m_SelectedFile = path;
                        m_PreviewLoaded = false;
                        m_PreviewContent.clear();
                        // Load text preview (limit 8KB)
                        std::ifstream f(path, std::ios::binary);
                        if (f)
                        {
                            std::ostringstream ss;
                            ss << f.rdbuf();
                            m_PreviewContent = ss.str();
                            if (m_PreviewContent.size() > 8192)
                                m_PreviewContent = m_PreviewContent.substr(0, 8192) + "\n... [truncated]";
                            m_PreviewLoaded = true;
                        }
                    }
                    // Double click: open in system editor
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        OpenInSystemEditor(path);
                    }
                }
            }
            ImGui::PopStyleColor();
        }
    }
    catch (const std::exception& e)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
    }
    ImGui::EndChild();

    // --- Preview Panel ---
    if (m_PreviewLoaded)
    {
        ImGui::SameLine();
        ImGui::BeginChild("FilePreview", ImVec2(0, 0), true);
        ImGui::TextDisabled("%s", m_SelectedFile.filename().string().c_str());
        ImGui::Separator();
        ImGui::InputTextMultiline("##preview", const_cast<char*>(m_PreviewContent.c_str()), m_PreviewContent.size() + 1,
                                  ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndChild();
    }

    ImGui::End();
}
#endif
