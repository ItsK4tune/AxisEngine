#pragma once
#ifdef ENABLE_EDITOR
#include <core/logic/logger_types.h>
#include <editor/i_editor_panel.h>
#include <mutex>
#include <string>
#include <deque>

struct Scene;

struct LogEntry
{
    LogType type;
    std::string tag;
    std::string message;
};

class ConsolePanel : public IEditorPanel
{
public:
    static ConsolePanel* s_Instance;

    ~ConsolePanel() override;

    void Initialize() override;
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "Console [Ctrl+7]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Debug;
    }

    // Called from LogManager hook
    void PushLog(LogType type, const std::string& tag, const std::string& message);

private:
    std::deque<LogEntry> m_Logs;
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
