#include "utils/logger.h"
#include <iostream>
#include <iomanip>

#ifdef AXIS_LOG_LEVEL_DEF
    #if AXIS_LOG_LEVEL_DEF == 0
        LogLevel Logger::s_CurrentLevel = LogLevel::None;
    #elif AXIS_LOG_LEVEL_DEF == 1
        LogLevel Logger::s_CurrentLevel = LogLevel::Minimal;
    #elif AXIS_LOG_LEVEL_DEF == 2
        LogLevel Logger::s_CurrentLevel = LogLevel::Flex;
    #else
        LogLevel Logger::s_CurrentLevel = LogLevel::Debug;
    #endif
#else
    LogLevel Logger::s_CurrentLevel = LogLevel::Minimal;
#endif
std::mutex Logger::s_LogMutex;

void Logger::Initialize() {
}

void Logger::SetLogLevel(LogLevel level) {
    s_CurrentLevel = level;
}

LogLevel Logger::GetLogLevel() {
    return s_CurrentLevel;
}

void Logger::Log(LogType type, const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(s_LogMutex);

    std::string levelStr;
    std::ostream* outStream = &std::cout;

    switch (type) {
        case LogType::Info:
            levelStr = "INFO";
            break;
        case LogType::Warning:
            levelStr = "WARNING";
            break;
        case LogType::Error:
            levelStr = "ERROR";
            outStream = &std::cerr;
            break;
        case LogType::Debug:
            levelStr = "DEBUG";
            break;
    }

    (*outStream) << "[" << levelStr << "] [" << tag << "] " << message << std::endl;
}
