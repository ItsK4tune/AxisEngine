#include <platform/strategy/macos/macos_runtime.h>

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <sstream>
#include <utility>

namespace
{
IPlatformRuntime::CrashReportCallback g_CrashReportCallback;

void FatalSignalHandler(int signal)
{
    std::ostringstream report;
    report << "Fatal signal caught: " << signal << "\n";

    void* frames[64] = {};
    int frameCount = backtrace(frames, 64);
    char** symbols = backtrace_symbols(frames, frameCount);
    if (symbols)
    {
        report << "Stack Trace:\n";
        for (int i = 0; i < frameCount; ++i)
        {
            report << "  #" << i << " " << symbols[i] << "\n";
        }
        free(symbols);
    }

    if (g_CrashReportCallback)
        g_CrashReportCallback(report.str());

    _exit(128 + signal);
}
}  // namespace

const char* MacRuntime::GetName() const
{
    return "macOS";
}

void MacRuntime::DetachConsole()
{
}

void MacRuntime::InstallCrashHandler(CrashReportCallback callback)
{
    g_CrashReportCallback = std::move(callback);
    signal(SIGABRT, FatalSignalHandler);
    signal(SIGBUS, FatalSignalHandler);
    signal(SIGFPE, FatalSignalHandler);
    signal(SIGILL, FatalSignalHandler);
    signal(SIGSEGV, FatalSignalHandler);
}

std::tm MacRuntime::LocalTime(std::time_t time) const
{
    std::tm result{};
    localtime_r(&time, &result);
    return result;
}
