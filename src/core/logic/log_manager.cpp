#include <core/logic/log_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <platform/interface/i_platform_runtime.h>
#include <platform/logic/platform_services.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>

void LogManager::Initialize(LogLevel level)
{
    SetLogLevel(level);

    if (level == LogLevel::None)
    {
#ifndef ENABLE_EDITOR
        GetNativePlatformRuntime().DetachConsole();
#endif
    }
    else
    {
#ifndef ENABLE_EDITOR
        GetNativePlatformRuntime().DetachConsole();
#endif
        std::string logsDirStr = FileSystem::getPath("logs");
        namespace fs = std::filesystem;
        fs::path logsDir = fs::path(logsDirStr);

        if (!fs::is_directory(logsDir))
        {
            fs::create_directories(logsDir);
        }

        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm bt = GetNativePlatformRuntime().LocalTime(timeT);

        std::stringstream ss;
        ss << "log_" << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S") << ".log";
        fs::path logPath = logsDir / ss.str();

        m_LogFile.open(logPath);
        if (m_LogFile.is_open())
        {
#ifdef ENABLE_EDITOR
            m_TeeOut = std::make_unique<TeeBuf>(std::cout.rdbuf(), m_LogFile.rdbuf());
            m_TeeErr = std::make_unique<TeeBuf>(std::cerr.rdbuf(), m_LogFile.rdbuf());

            m_OldOut = std::cout.rdbuf(m_TeeOut.get());
            m_OldErr = std::cerr.rdbuf(m_TeeErr.get());
#else
            m_OldOut = std::cout.rdbuf(m_LogFile.rdbuf());
            m_OldErr = std::cerr.rdbuf(m_LogFile.rdbuf());
#endif

            LOGGER_INFO("Application") << "Logging started at " << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
            LOGGER_INFO("Application") << "Log file: " << logPath.string();
        }
    }

    GetNativePlatformRuntime().InstallCrashHandler(
        [this](const std::string& report) { this->WriteCrashReport(report); });

    if (auto* es = ServiceLocator::Instance().Resolve<EventManager>())
    {
        m_ConfigListenerId =
            es->Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& ev) { this->OnConfigChanged(ev); });
    }
}

void LogManager::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_LogMutex);
#ifdef ENABLE_EDITOR
        m_EditorLogCallback = nullptr;
#endif
    }
    if (m_ConfigListenerId != -1)
    {
        if (auto* es = ServiceLocator::Instance().Resolve<EventManager>())
        {
            es->Unsubscribe<ConfigChangedEvent>(m_ConfigListenerId);
        }
        m_ConfigListenerId = -1;
    }
    if (m_OldOut)
        std::cout.rdbuf(m_OldOut);
    if (m_OldErr)
        std::cerr.rdbuf(m_OldErr);
    if (m_LogFile.is_open())
        m_LogFile.close();
}

void LogManager::OnConfigChanged(const ConfigChangedEvent& event)
{
    if (event.bitmask & (ConfigChangedEvent::General | ConfigChangedEvent::All))
    {
        SetLogLevel(event.config.logLevel);
    }
}

void LogManager::Log(LogType type, const std::string& tag, const std::string& message)
{
    if (m_LogLevel == LogLevel::None)
    {
        if (type == LogType::Error)
        {
            throw std::runtime_error("[" + tag + "] " + message);
        }
        return;
    }

    if (m_LogLevel == LogLevel::Minimal && type != LogType::Error)
        return;
    if (m_LogLevel == LogLevel::Flex && (type == LogType::Info || type == LogType::Debug))
        return;
    if (m_LogLevel == LogLevel::Verbose && type == LogType::Debug)
        return;

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

#ifdef ENABLE_EDITOR
    // Feed editor console panel
    if (m_EditorLogCallback)
    {
        m_EditorLogCallback(type, tag, message);
    }
#endif
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
