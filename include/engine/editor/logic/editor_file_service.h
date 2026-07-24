#pragma once

#ifdef ENABLE_EDITOR
#include <filesystem>
#include <string>
#include <string_view>

struct EditorFileResult
{
    bool success = false;
    std::filesystem::path path;
    std::string message;
};

class EditorFileService
{
public:
    static bool IsWithinRoot(const std::filesystem::path& root, const std::filesystem::path& target);
    static EditorFileResult CreateProjectDirectory(const std::filesystem::path& root,
                                                   const std::filesystem::path& path);
    static EditorFileResult CreateAssetFile(const std::filesystem::path& root,
                                            const std::filesystem::path& path, std::string_view content);
    static EditorFileResult DuplicateFile(const std::filesystem::path& root,
                                          const std::filesystem::path& source);
    static EditorFileResult Rename(const std::filesystem::path& root, const std::filesystem::path& source,
                                   const std::filesystem::path& destination);
    static EditorFileResult Remove(const std::filesystem::path& root, const std::filesystem::path& path);
};
#endif
