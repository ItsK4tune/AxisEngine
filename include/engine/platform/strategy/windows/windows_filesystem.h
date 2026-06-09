#pragma once

#include <platform/interface/i_platform_filesystem.h>

class WindowsFileSystem final : public IPlatformFileSystem
{
public:
    std::string GetExecutablePath() const override;
    std::string NormalizePath(const std::string& path) const override;
    bool IsAbsolutePath(const std::string& path) const override;
};
