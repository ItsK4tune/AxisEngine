#include "test_framework.h"

#include <core/logic/logger.h>
#include <cstdlib>
#include <sstream>

#if defined(_WIN32)
#include <crtdbg.h>
#include <windows.h>
#endif

namespace
{
void EnableConsoleColors()
{
    auto& colors = axis_test::ConsoleColors();
    colors.enabled = std::getenv("NO_COLOR") == nullptr;

#if defined(_WIN32)
    if (!colors.enabled)
        return;

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE)
    {
        colors.enabled = false;
        return;
    }

    DWORD mode = 0;
    if (!GetConsoleMode(output, &mode))
    {
        colors.enabled = false;
        return;
    }

    if (!SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        colors.enabled = false;
    }
#endif
}

const char* Color(const char* value)
{
    return axis_test::ConsoleColors().enabled ? value : "";
}

class ScopedStreamCapture
{
public:
    ScopedStreamCapture()
    {
        m_OldOut = std::cout.rdbuf(m_Out.rdbuf());
        m_OldErr = std::cerr.rdbuf(m_Err.rdbuf());
    }

    ~ScopedStreamCapture()
    {
        Stop();
    }

    std::string Stop()
    {
        if (!m_Stopped)
        {
            std::cout.rdbuf(m_OldOut);
            std::cerr.rdbuf(m_OldErr);
            m_Stopped = true;
        }
        return m_Out.str() + m_Err.str();
    }

private:
    std::ostringstream m_Out;
    std::ostringstream m_Err;
    std::streambuf* m_OldOut = nullptr;
    std::streambuf* m_OldErr = nullptr;
    bool m_Stopped = false;
};

int CountErrorLogs(const std::string& text)
{
    int count = 0;
    std::size_t pos = 0;
    while ((pos = text.find("[ERROR]", pos)) != std::string::npos)
    {
        ++count;
        pos += 7;
    }
    return count;
}

void PrintCapturedLogs(const std::string& text)
{
    if (text.empty())
        return;

    const auto& colors = axis_test::ConsoleColors();
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line))
    {
        if (line.empty())
            continue;

        const bool isError = line.find("[ERROR]") != std::string::npos;
        const bool isWarning = line.find("[WARNING]") != std::string::npos;
        const char* color = isError ? colors.red : (isWarning ? colors.yellow : "");
        std::cout << "       " << Color(color) << line << Color(colors.reset) << std::endl;
    }
}
}  // namespace

int main(int argc, char** argv)
{
#if defined(_WIN32)
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
    Logger::SetLogLevel(LogLevel::Minimal);
    EnableConsoleColors();

    int failed = 0;
    int passed = 0;
    int selected = 0;
    const std::string filter = argc > 1 ? argv[1] : "";
    const auto& tests = axis_test::Registry();
    const auto& colors = axis_test::ConsoleColors();

    std::cout << "Running " << tests.size() << " AxisEngine tests";
    if (!filter.empty())
        std::cout << " matching \"" << filter << "\"";
    std::cout << std::endl;
    for (const auto& test : tests)
    {
        if (!filter.empty() && std::string(test.name).find(filter) == std::string::npos)
            continue;

        ++selected;
        axis_test::ResetExpectedErrorLogs();
        std::string capturedLogs;
        try
        {
            std::cout << Color(colors.yellow) << "[RUN ]" << Color(colors.reset) << " " << test.name << std::endl;
            {
                ScopedStreamCapture capture;
                try
                {
                    test.fn();
                }
                catch (...)
                {
                    capturedLogs = capture.Stop();
                    throw;
                }
                capturedLogs = capture.Stop();
            }

            const int expectedErrors = axis_test::ConsumeExpectedErrorLogs();
            const int actualErrors = CountErrorLogs(capturedLogs);
            if (actualErrors != expectedErrors)
            {
                std::ostringstream message;
                message << "unexpected error log count: expected " << expectedErrors << ", got " << actualErrors;
                throw axis_test::Failure(message.str());
            }

            ++passed;
            std::cout << Color(colors.green) << "[PASS]" << Color(colors.reset) << " " << test.name << std::endl;
        }
        catch (const std::exception& e)
        {
            axis_test::ConsumeExpectedErrorLogs();
            ++failed;
            std::cout << Color(colors.red) << "[FAIL]" << Color(colors.reset) << " " << test.name << "\n       "
                      << e.what() << std::endl;
            PrintCapturedLogs(capturedLogs);
        }
        catch (...)
        {
            axis_test::ConsumeExpectedErrorLogs();
            ++failed;
            std::cout << Color(colors.red) << "[FAIL]" << Color(colors.reset) << " " << test.name
                      << "\n       unknown exception" << std::endl;
            PrintCapturedLogs(capturedLogs);
        }
    }

    if (failed > 0)
    {
        std::cout << Color(colors.red) << "FAILED" << Color(colors.reset) << ": " << failed << " failed, " << passed
                  << " passed, " << tests.size() << " total" << std::endl;
        return 1;
    }

    std::cout << Color(colors.green) << "PASSED" << Color(colors.reset) << ": " << passed << " passed, "
              << selected << " run, " << tests.size() << " total" << std::endl;
    return 0;
}
