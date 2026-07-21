#include <core/logic/log_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <platform/interface/i_platform_runtime.h>
#include <platform/logic/platform_services.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

void LogManager::Initialize(LogLevel level)
{
    SetLogLevel(level);

    if (level == LogLevel::None)
    {
    }
    else
    {
        std::string logsDirStr = FileSystem::getPath("logs");
        namespace fs = std::filesystem;
        fs::path logsDir = fs::path(logsDirStr);

        if (!fs::is_directory(logsDir))
        {
            fs::create_directories(logsDir);
        }

        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm bt = AcquirePlatformRuntime()->LocalTime(timeT);

        std::stringstream ss;
        ss << "log_" << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S") << ".log";
        fs::path logPath = logsDir / ss.str();

        m_LogFile.open(logPath);
        if (m_LogFile.is_open())
        {
            m_TeeOut = std::make_unique<TeeBuf>(std::cout.rdbuf(), m_LogFile.rdbuf());
            m_TeeErr = std::make_unique<TeeBuf>(std::cerr.rdbuf(), m_LogFile.rdbuf());

            m_OldOut = std::cout.rdbuf(m_TeeOut.get());
            m_OldErr = std::cerr.rdbuf(m_TeeErr.get());

            LOGGER_INFO("Application") << "Logging started at " << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
            LOGGER_INFO("Application") << "Log file: " << logPath.string();
        }
    }

    AcquirePlatformRuntime()->InstallCrashHandler(
        [this](const std::string& report) { this->WriteCrashReport(report); });

    m_ConfigListenerId = EventManager::Instance().Subscribe<ConfigChangedEvent>(
        [this](const ConfigChangedEvent& ev) { this->OnConfigChanged(ev); });
}

void LogManager::Shutdown()
{
    if (m_ConfigListenerId != -1)
    {
        EventManager::Instance().Unsubscribe<ConfigChangedEvent>(m_ConfigListenerId);
        m_ConfigListenerId = -1;
    }
    if (m_OldOut)
    {
        std::cout.rdbuf(m_OldOut);
        m_OldOut = nullptr;
    }
    if (m_OldErr)
    {
        std::cerr.rdbuf(m_OldErr);
        m_OldErr = nullptr;
    }
    if (m_LogFile.is_open())
        m_LogFile.close();
    m_TeeOut.reset();
    m_TeeErr.reset();
}

void LogManager::OnConfigChanged(const ConfigChangedEvent& event)
{
    if (HasConfigChanged(event, ConfigChangedEvent::General))
    {
        SetLogLevel(event.config.logLevel);
    }
}

void LogManager::Log(LogType type, const std::string& tag, const std::string& message)
{
    const LogLevel level = m_LogLevel.load(std::memory_order_relaxed);
    if (level == LogLevel::None)
        return;

    if (level == LogLevel::Minimal && type != LogType::Error)
        return;
    if (level == LogLevel::Flex && (type == LogType::Info || type == LogType::Debug))
        return;
    if (level == LogLevel::Verbose && type == LogType::Debug)
        return;

    LogCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_LogMutex);

        std::string levelStr;
        std::ostream* outStream = &std::cout;

        switch (type)
        {
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

        callback = m_LogCallback;
    }
    if (callback)
        callback(type, tag, message);
}

void LogManager::WriteCrashReport(const std::string& report)
{
    if (m_LogFile.is_open())
    {
        m_LogFile << report;
        if (report.empty() || report.back() != '\n')
            m_LogFile << '\n';
        m_LogFile.flush();
        return;
    }

    std::cerr << report;
    if (report.empty() || report.back() != '\n')
        std::cerr << std::endl;
}
