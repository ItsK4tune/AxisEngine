#include <platform/strategy/posix/posix_filesystem.h>

#include <limits.h>
#include <unistd.h>

#include <algorithm>
#include <string>

std::string PosixFileSystem::GetExecutablePath() const
{
    char exePath[PATH_MAX] = {};
    ssize_t length = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (length == -1)
        return {};

    exePath[length] = '\0';
    return NormalizePath(exePath);
}

std::string PosixFileSystem::NormalizePath(const std::string& path) const
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool PosixFileSystem::IsAbsolutePath(const std::string& path) const
{
    std::string normalized = NormalizePath(path);
    return !normalized.empty() && normalized[0] == '/';
}
