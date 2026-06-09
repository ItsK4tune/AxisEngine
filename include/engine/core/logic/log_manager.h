#pragma once

#include <core/logic/logger_types.h>
#include <core/type/event_types.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

class TeeBuf : public std::streambuf
{
public:
    TeeBuf(std::streambuf* sb1, std::streambuf* sb2) : sb1(sb1), sb2(sb2)
    {
    }

protected:
    virtual int overflow(int c) override
    {
        if (c == EOF)
            return !EOF;
        int const r1 = sb1->sputc(c);
        int const r2 = sb2->sputc(c);
        return r1 == EOF || r2 == EOF ? EOF : c;
    }
    virtual int sync() override
    {
        int const r1 = sb1->pubsync();
        int const r2 = sb2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }

private:
    std::streambuf *sb1, *sb2;
};

class LogManager
{
public:
    static LogManager& Instance()
    {
        static LogManager instance;
        return instance;
    }

    void Initialize(LogLevel level);
    void Shutdown();

    void SetLogLevel(LogLevel level)
    {
        m_LogLevel = level;
    }
    LogLevel GetLogLevel() const
    {
        return m_LogLevel;
    }

    void Log(LogType type, const std::string& tag, const std::string& message);

#ifdef ENABLE_EDITOR
    using LogCallback = std::function<void(LogType, const std::string&, const std::string&)>;
    void SetEditorLogCallback(LogCallback cb)
    {
        m_EditorLogCallback = std::move(cb);
    }
#endif

private:
    LogManager() = default;
    ~LogManager() = default;

    void OnConfigChanged(const ConfigChangedEvent& event);
    void WriteCrashReport(const std::string& report);

    std::ofstream m_LogFile;
    std::unique_ptr<TeeBuf> m_TeeOut;
    std::unique_ptr<TeeBuf> m_TeeErr;
    std::streambuf* m_OldOut = nullptr;
    std::streambuf* m_OldErr = nullptr;
    int m_ConfigListenerId = -1;
    LogLevel m_LogLevel = LogLevel::None;
    std::mutex m_LogMutex;
#ifdef ENABLE_EDITOR
    LogCallback m_EditorLogCallback;
#endif
};
