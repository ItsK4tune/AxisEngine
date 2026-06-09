#pragma once

#include <string>

class IPlatformFileSystem
{
public:
    virtual ~IPlatformFileSystem() = default;

    virtual std::string GetExecutablePath() const = 0;
    virtual std::string NormalizePath(const std::string& path) const = 0;
    virtual bool IsAbsolutePath(const std::string& path) const = 0;
};
