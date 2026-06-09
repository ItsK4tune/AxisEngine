#include <platform/strategy/windows/windows_filesystem.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <string>

std::string WindowsFileSystem::GetExecutablePath() const
{
    std::string buffer(MAX_PATH, '\0');
    DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0)
        return {};

    while (length == buffer.size())
    {
        buffer.resize(buffer.size() * 2);
        length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
            return {};
    }

    buffer.resize(length);
    return NormalizePath(buffer);
}

std::string WindowsFileSystem::NormalizePath(const std::string& path) const
{
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool WindowsFileSystem::IsAbsolutePath(const std::string& path) const
{
    std::string normalized = NormalizePath(path);
    if (normalized.size() >= 3 && normalized[1] == ':' && normalized[2] == '/')
        return true;
    if (normalized.rfind("//", 0) == 0)
        return true;
    return !normalized.empty() && normalized[0] == '/';
}
