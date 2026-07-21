#pragma once

#include <core/logic/logger_types.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

class Logger
{
public:
    static void SetLogLevel(LogLevel level);
    static LogLevel GetLogLevel();

    static void Log(LogType type, const std::string& tag, const std::string& message);

    template <typename T>
    static void LogStream(std::stringstream& ss, T&& t)
    {
        ss << std::forward<T>(t);
    }

    template <typename T, typename... Args>
    static void LogStream(std::stringstream& ss, T&& t, Args&&... args)
    {
        ss << std::forward<T>(t);
        LogStream(ss, std::forward<Args>(args)...);
    }
};

class LogStreamer
{
public:
    LogStreamer(LogType type, const std::string& tag) : m_Type(type), m_Tag(tag)
    {
    }

    ~LogStreamer()
    {
        Logger::Log(m_Type, m_Tag, m_Stream.str());
    }

    template <typename T>
    LogStreamer& operator<<(const T& value)
    {
        m_Stream << value;
        return *this;
    }

private:
    LogType m_Type;
    std::string m_Tag;
    std::stringstream m_Stream;
};

#define LOGGER_ERROR(Tag) LogStreamer(LogType::Error, Tag)
#define LOGGER_WARN(Tag) LogStreamer(LogType::Warning, Tag)
#define LOGGER_INFO(Tag) LogStreamer(LogType::Info, Tag)
#define LOGGER_LOG(Tag) LogStreamer(LogType::Info, Tag)
#define LOGGER_DEBUG(Tag) LogStreamer(LogType::Debug, Tag)
