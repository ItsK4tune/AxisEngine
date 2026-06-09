#pragma once

#include <platform/interface/i_platform_runtime.h>

class PosixRuntime final : public IPlatformRuntime
{
public:
    const char* GetName() const override;
    void DetachConsole() override;
    void InstallCrashHandler(CrashReportCallback callback) override;
    std::tm LocalTime(std::time_t time) const override;
};
