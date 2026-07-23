#include <editor/panels/console_panel.h>

#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <vector>

ConsolePanel* ConsolePanel::s_Instance = nullptr;

ConsolePanel::~ConsolePanel()
{
    if (s_Instance == this)
        s_Instance = nullptr;
}

void ConsolePanel::Initialize()
{
    s_Instance = this;
}

void ConsolePanel::PushLog(LogType type, const std::string& tag, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Logs.size() >= MAX_LOG_ENTRIES)
        m_Logs.pop_front();
    m_Logs.push_back({type, tag, message});
}

void ConsolePanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    // Toolbar
    if (ImGui::Button("Clear"))
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Logs.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-Scroll", &m_AutoScroll);
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Checkbox("Info", &m_ShowInfo);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.2f, 1.0f));
    ImGui::Checkbox("Warn", &m_ShowWarnings);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::Checkbox("Error", &m_ShowErrors);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
    ImGui::Checkbox("Debug", &m_ShowDebug);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputTextWithHint("##Filter", "Filter...", m_FilterBuf, IM_ARRAYSIZE(m_FilterBuf));

    ImGui::Separator();

    // Snapshot under the mutex, then format and render without blocking log producers.
    std::vector<LogEntry> logs;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        logs.assign(m_Logs.begin(), m_Logs.end());
    }

    // Log list
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    {
        std::string filterStr = m_FilterBuf;
        std::string filterLower = filterStr;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

        for (const auto& entry : logs)
        {
            // Type filter
            if (entry.type == LogType::Info && !m_ShowInfo)
                continue;
            if (entry.type == LogType::Warning && !m_ShowWarnings)
                continue;
            if (entry.type == LogType::Error && !m_ShowErrors)
                continue;
            if (entry.type == LogType::Debug && !m_ShowDebug)
                continue;

            // Text filter
            if (!filterLower.empty())
            {
                std::string combined = entry.tag + " " + entry.message;
                std::string combinedLower = combined;
                std::transform(combinedLower.begin(), combinedLower.end(), combinedLower.begin(),
                               [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
                if (combinedLower.find(filterLower) == std::string::npos)
                    continue;
            }

            ImVec4 color;
            const char* prefix;
            switch (entry.type)
            {
                case LogType::Error:
                    color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    prefix = "[ERR]";
                    break;
                case LogType::Warning:
                    color = ImVec4(1.0f, 0.9f, 0.2f, 1.0f);
                    prefix = "[WRN]";
                    break;
                case LogType::Debug:
                    color = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
                    prefix = "[DBG]";
                    break;
                default:
                    color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                    prefix = "[INF]";
                    break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextWrapped("%s [%s] %s", prefix, entry.tag.c_str(), entry.message.c_str());
            ImGui::PopStyleColor();
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
#endif
