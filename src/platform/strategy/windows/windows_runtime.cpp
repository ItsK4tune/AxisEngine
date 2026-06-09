#include <platform/strategy/windows/windows_runtime.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <DbgHelp.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <utility>

namespace
{
IPlatformRuntime::CrashReportCallback g_CrashReportCallback;

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    std::ostringstream report;
    report << "Unhandled Exception Caught!\n";
    if (exceptionInfo && exceptionInfo->ExceptionRecord)
    {
        report << "Exception Code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::dec << "\n";
        report << "Faulting Address: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress << std::dec
               << "\n";
    }

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    if (SymInitialize(process, nullptr, TRUE) && exceptionInfo && exceptionInfo->ContextRecord)
    {
        CONTEXT context = *exceptionInfo->ContextRecord;
        STACKFRAME64 frame{};
        DWORD machineType = 0;

#if defined(_M_X64) || defined(__x86_64__)
        machineType = IMAGE_FILE_MACHINE_AMD64;
        frame.AddrPC.Offset = context.Rip;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrStack.Offset = context.Rsp;
#elif defined(_M_IX86) || defined(__i386__)
        machineType = IMAGE_FILE_MACHINE_I386;
        frame.AddrPC.Offset = context.Eip;
        frame.AddrFrame.Offset = context.Ebp;
        frame.AddrStack.Offset = context.Esp;
#else
        machineType = IMAGE_FILE_MACHINE_AMD64;
#endif

        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;

        report << "Stack Trace:\n";
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

            report << "  #" << frameIndex << " 0x" << std::hex << frame.AddrPC.Offset;
            if (moduleBase)
            {
                report << " " << moduleName.data() << "+0x" << (frame.AddrPC.Offset - moduleBase);
            }
            report << std::dec;

            if (hasSymbol)
            {
                report << " " << symbol->Name << "+0x" << std::hex << displacement << std::dec;
            }
            if (hasLine)
            {
                report << " (" << line.FileName << ":" << line.LineNumber << ")";
            }
            report << "\n";
        }
        SymCleanup(process);
    }

    if (g_CrashReportCallback)
        g_CrashReportCallback(report.str());

    return EXCEPTION_EXECUTE_HANDLER;
}
}  // namespace

const char* WindowsRuntime::GetName() const
{
    return "Windows";
}

void WindowsRuntime::DetachConsole()
{
    FreeConsole();
}

void WindowsRuntime::InstallCrashHandler(CrashReportCallback callback)
{
    g_CrashReportCallback = std::move(callback);
    SetUnhandledExceptionFilter(CrashHandler);
}

std::tm WindowsRuntime::LocalTime(std::time_t time) const
{
    std::tm result{};
    localtime_s(&result, &time);
    return result;
}
