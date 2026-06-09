#pragma once

#include <ctime>
#include <functional>
#include <string>

class IPlatformRuntime
{
public:
    using CrashReportCallback = std::function<void(const std::string&)>;

    virtual ~IPlatformRuntime() = default;

    virtual const char* GetName() const = 0;
    virtual void DetachConsole() = 0;
    virtual void InstallCrashHandler(CrashReportCallback callback) = 0;
    virtual std::tm LocalTime(std::time_t time) const = 0;
};
