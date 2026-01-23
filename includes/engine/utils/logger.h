#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <sstream>

enum class LogLevel {
    None,
    Minimal,
    Flex,
    Debug
};

enum class LogType {
    Info,
    Warning,
    Error,
    Debug
};

class Logger {
public:
    static void Initialize();
    static void SetLogLevel(LogLevel level);
    static LogLevel GetLogLevel();

    static void Log(LogType type, const std::string& tag, const std::string& message);

    template <typename T>
    static void LogStream(std::stringstream& ss, T&& t) {
        ss << std::forward<T>(t);
    }

    template <typename T, typename... Args>
    static void LogStream(std::stringstream& ss, T&& t, Args&&... args) {
        ss << std::forward<T>(t);
        LogStream(ss, std::forward<Args>(args)...);
    }

private:
    static LogLevel s_CurrentLevel;
    static std::mutex s_LogMutex;
};

class LogStreamer {
public:
    LogStreamer(LogType type, const std::string& tag) 
        : m_Type(type), m_Tag(tag) {}

    ~LogStreamer() {
        Logger::Log(m_Type, m_Tag, m_Stream.str());
    }

    template<typename T>
    LogStreamer& operator<<(const T& value) {
        m_Stream << value;
        return *this;
    }

private:
    LogType m_Type;
    std::string m_Tag;
    std::stringstream m_Stream;
};

#ifndef AXIS_LOG_LEVEL_DEF
#define AXIS_LOG_LEVEL_DEF 1
#endif

#if AXIS_LOG_LEVEL_DEF >= 1
    #define LOGGER_WARN(Tag)   LogStreamer(LogType::Warning, Tag)
    #define LOGGER_ERROR(Tag)  LogStreamer(LogType::Error, Tag)
#else
    #define LOGGER_WARN(Tag)   if(true); else LogStreamer(LogType::Warning, Tag)
    #define LOGGER_ERROR(Tag)  if(true); else LogStreamer(LogType::Error, Tag)
#endif 

// Define INFO macros
#define LOGGER_LOG(Tag)    if (AXIS_LOG_LEVEL_DEF < 2) ; else LogStreamer(LogType::Info, Tag)
#define LOGGER_INFO(Tag)   if (AXIS_LOG_LEVEL_DEF < 2) ; else LogStreamer(LogType::Info, Tag)

// Define DEBUG macros
#define LOGGER_DEBUG(Tag)  if (AXIS_LOG_LEVEL_DEF < 3) ; else LogStreamer(LogType::Debug, Tag)
