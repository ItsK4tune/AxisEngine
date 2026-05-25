#include <core/logic/log_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <DbgHelp.h>
#include <array>
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
        FreeConsole();
#endif
    }
    else
    {
#ifndef ENABLE_EDITOR
        FreeConsole();
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
        std::tm bt{};
        localtime_s(&bt, &timeT);

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

    SetUnhandledExceptionFilter(CrashHandler);

    if (auto* es = ServiceLocator::Instance().Resolve<EventManager>())
    {
        m_ConfigListenerId =
            es->Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& ev) { this->OnConfigChanged(ev); });
    }
}

void LogManager::Shutdown()
{
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

LONG WINAPI LogManager::CrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    auto& logFile = Instance().m_LogFile;
    if (logFile.is_open())
    {
        logFile << "Unhandled Exception Caught!\n";
        if (exceptionInfo && exceptionInfo->ExceptionRecord)
        {
            logFile << "Exception Code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::dec
                    << "\n";
            logFile << "Faulting Address: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress
                    << std::dec << "\n";
        }

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

        if (SymInitialize(process, nullptr, TRUE) && exceptionInfo && exceptionInfo->ContextRecord)
        {
            CONTEXT context = *exceptionInfo->ContextRecord;
            STACKFRAME64 frame{};
            DWORD machineType = IMAGE_FILE_MACHINE_AMD64;

            frame.AddrPC.Offset = context.Rip;
            frame.AddrPC.Mode = AddrModeFlat;
            frame.AddrFrame.Offset = context.Rbp;
            frame.AddrFrame.Mode = AddrModeFlat;
            frame.AddrStack.Offset = context.Rsp;
            frame.AddrStack.Mode = AddrModeFlat;

            logFile << "Stack Trace:\n";
            for (int frameIndex = 0; frameIndex < 64; ++frameIndex)
            {
                if (!StackWalk64(machineType, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64,
                                 SymGetModuleBase64, nullptr))
                {
                    break;
                }

                if (frame.AddrPC.Offset == 0)
                    break;

                DWORD64 moduleBase = SymGetModuleBase64(process, frame.AddrPC.Offset);
                std::array<char, MAX_PATH> moduleName{};
                if (moduleBase)
                {
                    GetModuleFileNameA(reinterpret_cast<HMODULE>(moduleBase), moduleName.data(),
                                       static_cast<DWORD>(moduleName.size()));
                }

                alignas(SYMBOL_INFO) std::array<unsigned char, sizeof(SYMBOL_INFO) + MAX_SYM_NAME> symbolBuffer{};
                auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer.data());
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = MAX_SYM_NAME;

                DWORD64 displacement = 0;
                bool hasSymbol = SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol) == TRUE;

                IMAGEHLP_LINE64 line{};
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                DWORD lineDisplacement = 0;
                bool hasLine = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line) == TRUE;

                logFile << "  #" << frameIndex << " 0x" << std::hex << frame.AddrPC.Offset;
                if (moduleBase)
                {
                    logFile << " " << moduleName.data() << "+0x" << (frame.AddrPC.Offset - moduleBase);
                }
                logFile << std::dec;

                if (hasSymbol)
                {
                    logFile << " " << symbol->Name << "+0x" << std::hex << displacement << std::dec;
                }
                if (hasLine)
                {
                    logFile << " (" << line.FileName << ":" << line.LineNumber << ")";
                }
                logFile << "\n";
            }
            SymCleanup(process);
        }
        logFile.flush();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
