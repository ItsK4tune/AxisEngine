#include <platform/strategy/macos/macos_filesystem.h>

#include <mach-o/dyld.h>
#include <limits.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

std::string MacFileSystem::GetExecutablePath() const
{
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0)
        return {};

    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
        return {};

    char resolved[PATH_MAX] = {};
    if (realpath(buffer.data(), resolved))
        return NormalizePath(resolved);

    return NormalizePath(std::string(buffer.data()));
}

std::string MacFileSystem::NormalizePath(const std::string& path) const
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool MacFileSystem::IsAbsolutePath(const std::string& path) const
{
    std::string normalized = NormalizePath(path);
    return !normalized.empty() && normalized[0] == '/';
}
