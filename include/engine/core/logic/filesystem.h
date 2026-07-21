#pragma once

#include <platform/interface/i_platform_filesystem.h>
#include <platform/logic/platform_services.h>
#include <filesystem>
#include <string>
#include <vector>

class FileSystem
{
public:
    static void setEngineAssetRoot(const std::string& path)
    {
        s_EngineAssetRoot = AcquirePlatformFileSystem()->NormalizePath(path);
    }

    static std::string getPath(const std::string& path)
    {
        static constexpr const char* AssetPrefix = "asset://";
        if (path.rfind(AssetPrefix, 0) == 0)
            return getEngineAssetPath(path.substr(std::char_traits<char>::length(AssetPrefix)));

        auto platform = AcquirePlatformFileSystem();
        std::string root = getRoot();
        std::string normPath = platform->NormalizePath(path);

        if (!root.empty() && (normPath == root || normPath.rfind(root + "/", 0) == 0))
            return normPath;

        if (platform->IsAbsolutePath(normPath))
            return normPath;

        if (!root.empty())
            return root + "/" + normPath;

        return normPath;
    }

    static std::string getEngineAssetPath(const std::string& relativePath)
    {
        auto platform = AcquirePlatformFileSystem();
        const std::string root = getRoot();
        const std::string relative = platform->NormalizePath(relativePath);
        const std::string separator = root.empty() ? "" : "/";
        std::vector<std::string> candidates;
        if (!s_EngineAssetRoot.empty())
            candidates.push_back(s_EngineAssetRoot + "/" + relative);
        candidates.push_back(root + separator + "share/AxisEngine/assets/" + relative);
        candidates.push_back(root + separator + "assets/" + relative);
        candidates.push_back(root + separator + "include/engine/asset/" + relative);

        for (const auto& candidate : candidates)
        {
            if (std::filesystem::exists(std::filesystem::u8path(candidate)))
                return platform->NormalizePath(candidate);
        }
        return platform->NormalizePath(candidates[0]);
    }

    static std::string getRelativePath(const std::string& path)
    {
        auto platform = AcquirePlatformFileSystem();
        std::string root = getRoot();
        if (root.empty())
            return path;

        std::string normPath = platform->NormalizePath(path);

        if (normPath == root || normPath.rfind(root + "/", 0) == 0)
        {
            std::string rel = normPath.substr(root.length());
            if (!rel.empty() && rel[0] == '/')
                rel = rel.substr(1);
            return rel;
        }
        return path;
    }

private:
    static inline std::string s_EngineAssetRoot;

    static std::string getRoot()
    {
        static const std::string cachedRoot = [] {
            auto platform = AcquirePlatformFileSystem();
            const std::string executable = platform->NormalizePath(platform->GetExecutablePath());
            if (executable.empty())
                return std::string{};

            std::filesystem::path directory = std::filesystem::u8path(executable).parent_path();
            for (std::filesystem::path cursor = directory; !cursor.empty(); cursor = cursor.parent_path())
            {
                const bool sourceRoot = std::filesystem::exists(cursor / "include" / "engine" / "asset");
                const bool installRoot = std::filesystem::exists(cursor / "share" / "AxisEngine" / "assets");
                if (sourceRoot || installRoot)
                    return platform->NormalizePath(cursor.generic_string());
                if (cursor == cursor.root_path())
                    break;
            }

            if (directory.filename() == "bin")
                directory = directory.parent_path();
            return platform->NormalizePath(directory.generic_string());
        }();
        return cachedRoot;
    }
};
