#include <editor/panels/file_hierarchy_panel.h>

#ifdef ENABLE_EDITOR
#include <core/logic/logger.h>
#include <editor/logic/editor_file_service.h>
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#endif

void FileHierarchyPanel::Initialize()
{
    std::error_code error;
    m_CurrentPath = std::filesystem::weakly_canonical(std::filesystem::current_path(), error);
    if (error)
        m_CurrentPath = std::filesystem::current_path();
    m_ProjectRoot = m_CurrentPath;
    m_PathInput = m_CurrentPath.string();
    m_History.push_back(m_CurrentPath);
    m_HistoryIndex = 0;
}

static bool NavigateTo(std::filesystem::path& current, std::string& input,
                       std::vector<std::filesystem::path>& history, size_t& histIdx,
                       const std::filesystem::path& projectRoot, const std::filesystem::path& target)
{
    if (!EditorFileService::IsWithinRoot(projectRoot, target))
        return false;
    std::error_code error;
    const std::filesystem::path normalized = std::filesystem::weakly_canonical(target, error);
    if (error || !std::filesystem::is_directory(normalized, error) || error)
        return false;
    if (histIdx + 1 < history.size())
        history.erase(history.begin() + histIdx + 1, history.end());
    current = normalized;
    input = normalized.string();
    history.push_back(normalized);
    histIdx = history.size() - 1;
    return true;
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
    DrawContents(scene);
    ImGui::End();
}

static bool IsSimpleAssetName(const char* value)
{
    const std::filesystem::path name(value ? value : "");
    return !name.empty() && name != "." && name != ".." && name == name.filename();
}

void FileHierarchyPanel::DrawContents(Scene& scene)
{
    if (ImGui::Button("New Folder"))
    {
        std::strncpy(m_AssetName.data(), "NewFolder", m_AssetName.size() - 1);
        m_OpenCreateFolder = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("New Asset"))
    {
        std::strncpy(m_AssetName.data(), "NewAsset", m_AssetName.size() - 1);
        m_OpenCreateAsset = true;
    }
    ImGui::SameLine();
    if (m_SelectedFile.empty())
        ImGui::BeginDisabled();
    if (ImGui::Button("Rename"))
    {
        std::strncpy(m_AssetName.data(), m_SelectedFile.filename().string().c_str(), m_AssetName.size() - 1);
        m_OpenRename = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Duplicate"))
    {
        const EditorFileResult result = EditorFileService::DuplicateFile(m_ProjectRoot, m_SelectedFile);
        m_OperationStatus = result.message;
        if (result.success)
            m_SelectedFile = result.path;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete"))
        m_OpenDelete = true;
    if (m_SelectedFile.empty())
        ImGui::EndDisabled();
    if (!m_OperationStatus.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", m_OperationStatus.c_str());
    }

    if (m_OpenCreateFolder)
    {
        ImGui::OpenPopup("Create folder");
        m_OpenCreateFolder = false;
    }
    if (ImGui::BeginPopupModal("Create folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Name", m_AssetName.data(), m_AssetName.size());
        if (ImGui::Button("Create"))
        {
            if (!IsSimpleAssetName(m_AssetName.data()))
                m_OperationStatus = "Use a simple folder name without path separators.";
            else
            {
                const EditorFileResult result =
                    EditorFileService::CreateProjectDirectory(m_ProjectRoot, m_CurrentPath / m_AssetName.data());
                m_OperationStatus = result.message;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (m_OpenCreateAsset)
    {
        ImGui::OpenPopup("Create asset");
        m_OpenCreateAsset = false;
    }
    if (ImGui::BeginPopupModal("Create asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Name", m_AssetName.data(), m_AssetName.size());
        const char* types[] = {"Scene", "Prefab / Fragment", "Material", "Text"};
        ImGui::Combo("Type", &m_NewAssetType, types, IM_ARRAYSIZE(types));
        if (ImGui::Button("Create"))
        {
            static constexpr const char* extensions[] = {".axs", ".prefab.axs", ".material.axs", ".txt"};
            static constexpr const char* templates[] = {
                "axis_scene:\n  Resources:\n  Entities:\n",
                "Entities:\n",
                "Material:\n  Roughness: 0.5\n  Metallic: 0.0\n  AO: 1.0\n",
                ""};
            if (!IsSimpleAssetName(m_AssetName.data()))
                m_OperationStatus = "Use a simple asset name without path separators.";
            else
            {
                std::filesystem::path path = m_CurrentPath / m_AssetName.data();
                if (path.extension().empty())
                    path += extensions[std::clamp(m_NewAssetType, 0, 3)];
                const EditorFileResult result = EditorFileService::CreateAssetFile(
                    m_ProjectRoot, path, templates[std::clamp(m_NewAssetType, 0, 3)]);
                m_OperationStatus = result.message;
                if (result.success)
                {
                    m_SelectedFile = result.path;
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (m_OpenRename)
    {
        ImGui::OpenPopup("Rename asset");
        m_OpenRename = false;
    }
    if (ImGui::BeginPopupModal("Rename asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("New name", m_AssetName.data(), m_AssetName.size());
        if (ImGui::Button("Rename"))
        {
            if (!IsSimpleAssetName(m_AssetName.data()))
                m_OperationStatus = "Use a simple asset name without path separators.";
            else
            {
                const auto destination = m_SelectedFile.parent_path() / m_AssetName.data();
                const EditorFileResult result =
                    EditorFileService::Rename(m_ProjectRoot, m_SelectedFile, destination);
                m_OperationStatus = result.message;
                if (result.success)
                    m_SelectedFile = result.path;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (m_OpenDelete)
    {
        ImGui::OpenPopup("Delete asset?");
        m_OpenDelete = false;
    }
    if (ImGui::BeginPopupModal("Delete asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("Delete '%s'? Non-empty folders are never removed.",
                           m_SelectedFile.string().c_str());
        if (ImGui::Button("Delete"))
        {
            const EditorFileResult result = EditorFileService::Remove(m_ProjectRoot, m_SelectedFile);
            m_OperationStatus = result.message;
            if (result.success)
            {
                m_SelectedFile.clear();
                m_PreviewLoaded = false;
                m_PreviewContent.clear();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Separator();
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
        if (m_CurrentPath != m_ProjectRoot && m_CurrentPath.has_parent_path())
        {
            if (NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, m_ProjectRoot,
                           m_CurrentPath.parent_path()))
            {
                m_SelectedFile.clear();
                m_PreviewContent.clear();
                m_PreviewLoaded = false;
            }
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
        if (NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, m_ProjectRoot, p))
        {
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
        if (NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, m_ProjectRoot, p))
        {
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

    float availW = ImGui::GetContentRegionAvail().x;
    float listW = m_PreviewLoaded ? availW * 0.5f : availW;

    ImGui::BeginChild("FileList", ImVec2(listW, 0), false);
    try
    {
        std::vector<std::filesystem::directory_entry> entries;
        for (auto& entry : std::filesystem::directory_iterator(m_CurrentPath))
        {
            if (EditorFileService::IsWithinRoot(m_ProjectRoot, entry.path()))
                entries.push_back(entry);
        }
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
                    m_SelectedFile = path;
                    m_PreviewLoaded = false;
                    m_PreviewContent.clear();
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        if (NavigateTo(m_CurrentPath, m_PathInput, m_History, m_HistoryIndex, m_ProjectRoot,
                                       path))
                        {
                            m_SelectedFile.clear();
                            m_PreviewContent.clear();
                            m_PreviewLoaded = false;
                        }
                    }
                }
                else
                {
                    if (m_SelectedFile != path)
                    {
                        m_SelectedFile = path;
                        m_PreviewLoaded = false;
                        m_PreviewContent.clear();
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

}
#endif
