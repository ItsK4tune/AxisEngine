#include <editor/panels/file_hierarchy_panel.h>

#ifdef ENABLE_EDITOR
#include <core/logic/logger.h>
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
    m_CurrentPath = std::filesystem::current_path();
    m_ProjectRoot = m_CurrentPath;
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
        std::error_code error;
        if (std::filesystem::is_regular_file(m_SelectedFile))
        {
            auto destination = m_SelectedFile.parent_path() /
                               (m_SelectedFile.stem().string() + "_copy" + m_SelectedFile.extension().string());
            std::filesystem::copy_file(m_SelectedFile, destination, std::filesystem::copy_options::overwrite_existing,
                                       error);
            m_OperationStatus = error ? error.message() : "Asset duplicated.";
        }
        else
            m_OperationStatus = "Only files can be duplicated.";
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
                std::error_code error;
                std::filesystem::create_directory(m_CurrentPath / m_AssetName.data(), error);
                m_OperationStatus = error ? error.message() : "Folder created.";
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
                std::ofstream output(path, std::ios::trunc);
                if (output)
                {
                    output << templates[std::clamp(m_NewAssetType, 0, 3)];
                    m_OperationStatus = "Asset created.";
                    m_SelectedFile = path;
                }
                else
                    m_OperationStatus = "Could not create asset.";
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
                std::error_code error;
                const auto destination = m_SelectedFile.parent_path() / m_AssetName.data();
                std::filesystem::rename(m_SelectedFile, destination, error);
                if (!error)
                    m_SelectedFile = destination;
                m_OperationStatus = error ? error.message() : "Asset renamed.";
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
        ImGui::TextWrapped("Delete '%s'? Non-empty folders are never removed.", m_SelectedFile.filename().string().c_str());
        if (ImGui::Button("Delete"))
        {
            std::error_code error;
            const bool removed = std::filesystem::remove(m_SelectedFile, error);
            m_OperationStatus = removed ? "Asset deleted." : (error ? error.message() : "Nothing was deleted.");
            if (removed)
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
                    m_SelectedFile = path;
                    m_PreviewLoaded = false;
                    m_PreviewContent.clear();
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

}
#endif
