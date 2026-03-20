#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <core/logic/logger_types.h>
#include <core/type/event_types.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/logic/event_system.h>

class TeeBuf : public std::streambuf {
public:
    TeeBuf(std::streambuf* sb1, std::streambuf* sb2) : sb1(sb1), sb2(sb2) {}
protected:
    virtual int overflow(int c) override {
        if (c == EOF) return !EOF;
        int const r1 = sb1->sputc(c);
        int const r2 = sb2->sputc(c);
        return r1 == EOF || r2 == EOF ? EOF : c;
    }
    virtual int sync() override {
        int const r1 = sb1->pubsync();
        int const r2 = sb2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }
private:
    std::streambuf *sb1, *sb2;
};

class LogManager {
public:
    static LogManager& Instance() {
        static LogManager instance;
        return instance;
    }

    void Initialize(LogLevel level);
    void Shutdown();

    void SetLogLevel(LogLevel level) { m_LogLevel = level; }
    LogLevel GetLogLevel() const { return m_LogLevel; }

    void Log(LogType type, const std::string& tag, const std::string& message);


private:
    LogManager() = default;
    ~LogManager() = default;

    static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exceptionInfo);
    void OnConfigChanged(const ConfigChangedEvent& event);

    std::ofstream m_LogFile;
    std::unique_ptr<TeeBuf> m_TeeOut;
    std::unique_ptr<TeeBuf> m_TeeErr;
    std::streambuf* m_OldOut = nullptr;
    std::streambuf* m_OldErr = nullptr;
    int m_ConfigListenerId = -1;
    LogLevel m_LogLevel = LogLevel::None;
    std::mutex m_LogMutex;
};
