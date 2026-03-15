#include <core/logic/log_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

void LogManager::Initialize(LogLevel level) {
    if (level == LogLevel::None) {
#ifndef ENABLE_DEBUG_SYSTEM
        FreeConsole();
#endif
        return;
    }

#ifndef ENABLE_DEBUG_SYSTEM
    FreeConsole();
#endif

    std::string logsDirStr = FileSystem::getPath("logs");
    namespace fs = std::filesystem;
    fs::path logsDir = fs::path(logsDirStr);

    if (!fs::is_directory(logsDir)) {
        fs::create_directories(logsDir);
    }

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
    localtime_s(&bt, &timeT);

    std::stringstream ss;
    ss << "log_" << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S") << ".txt";
    fs::path logPath = logsDir / ss.str();

    m_LogFile.open(logPath);
    if (m_LogFile.is_open()) {
#ifdef ENABLE_DEBUG_SYSTEM
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

    SetUnhandledExceptionFilter(CrashHandler);
}

void LogManager::Shutdown() {
    if (m_OldOut) std::cout.rdbuf(m_OldOut);
    if (m_OldErr) std::cerr.rdbuf(m_OldErr);
    if (m_LogFile.is_open()) m_LogFile.close();
}

LONG WINAPI LogManager::CrashHandler(EXCEPTION_POINTERS* exceptionInfo) {
    auto& logFile = Instance().m_LogFile;
    if (logFile.is_open()) {
        logFile << "Unhandled Exception Caught!\n";
        if (exceptionInfo && exceptionInfo->ExceptionRecord) {
            logFile << "Exception Code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::dec << "\n";
            logFile << "Faulting Address: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress << std::dec << "\n";
        }
        logFile.flush();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
