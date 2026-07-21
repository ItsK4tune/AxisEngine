#include <core/logic/logger.h>
#include <core/logic/log_manager.h>

void Logger::SetLogLevel(LogLevel level)
{
    LogManager::Instance().SetLogLevel(level);
}

LogLevel Logger::GetLogLevel()
{
    return LogManager::Instance().GetLogLevel();
}

void Logger::Log(LogType type, const std::string& tag, const std::string& message)
{
    LogManager::Instance().Log(type, tag, message);
}
