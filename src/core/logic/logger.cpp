#include <iomanip>
#include <iostream>
#include <stdexcept>
#include "core/logic/logger.h"

LogLevel Logger::s_CurrentLevel = LogLevel::None;
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
    if (s_CurrentLevel == LogLevel::None) {
        if (type == LogType::Error) {
            throw std::runtime_error("[" + tag + "] " + message);
        }
        return;
    }

    if (s_CurrentLevel == LogLevel::Minimal && type != LogType::Error) return;
    if (s_CurrentLevel == LogLevel::Flex && (type == LogType::Info || type == LogType::Debug)) return;
    if (s_CurrentLevel == LogLevel::Verbose && type == LogType::Debug) return;

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

    if (type == LogType::Error) {
        throw std::runtime_error("[" + tag + "] " + message);
    }
}
