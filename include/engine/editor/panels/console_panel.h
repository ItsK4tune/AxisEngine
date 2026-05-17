#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <core/logic/logger_types.h>
#include <string>
#include <vector>
#include <mutex>

struct Scene;

struct LogEntry {
    LogType type;
    std::string tag;
    std::string message;
};

class ConsolePanel : public IEditorPanel
{
public:
    static ConsolePanel* s_Instance;

    void Initialize() override;
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Console"; }
    PanelGroup GetGroup() const override { return PanelGroup::Debug; }

    // Called from LogManager hook
    void PushLog(LogType type, const std::string& tag, const std::string& message);

private:
    std::vector<LogEntry> m_Logs;
    std::mutex m_Mutex;
    bool m_AutoScroll = true;
    bool m_ShowInfo = true;
    bool m_ShowWarnings = true;
    bool m_ShowErrors = true;
    bool m_ShowDebug = false;
    char m_FilterBuf[256] = "";
    static constexpr size_t MAX_LOG_ENTRIES = 2000;
};
#endif
