#pragma once

#include <platform/interface/i_platform_filesystem.h>
#include <platform/logic/platform_services.h>
#include <string>

class FileSystem
{
public:
    static std::string getPath(const std::string& path)
    {
        auto& platform = GetNativePlatformFileSystem();
        std::string root = getRoot();
        std::string normPath = platform.NormalizePath(path);

        if (!root.empty() && normPath.find(root) == 0)
            return normPath;

        if (platform.IsAbsolutePath(normPath))
            return normPath;

        if (!root.empty())
            return root + "/" + normPath;

        return normPath;
    }

    static std::string getRelativePath(const std::string& path)
    {
        auto& platform = GetNativePlatformFileSystem();
        std::string root = getRoot();
        if (root.empty())
            return path;

        std::string normPath = platform.NormalizePath(path);

        if (normPath.find(root) == 0)
        {
            std::string rel = normPath.substr(root.length());
            if (!rel.empty() && rel[0] == '/')
                rel = rel.substr(1);
            return rel;
        }
        return path;
    }

private:
    static std::string getRoot()
    {
        static std::string cachedRoot;
        static bool initialized = false;

        if (initialized)
            return cachedRoot;

        initialized = true;

        auto& platform = GetNativePlatformFileSystem();
        std::string path = platform.NormalizePath(platform.GetExecutablePath());
        if (path.empty())
            return cachedRoot;

        size_t binPos = path.rfind("/bin/");
        if (binPos != std::string::npos)
        {
            cachedRoot = path.substr(0, binPos);
            size_t buildPos = cachedRoot.rfind("/build");
            if (buildPos != std::string::npos && buildPos + 6 == cachedRoot.length())
            {
                cachedRoot = cachedRoot.substr(0, buildPos);
            }
            return cachedRoot;
        }

        size_t buildPos = path.rfind("/build/");
        if (buildPos != std::string::npos)
        {
            cachedRoot = path.substr(0, buildPos);
            return cachedRoot;
        }

        size_t bundlePos = path.find(".app/Contents/MacOS/");
        if (bundlePos != std::string::npos)
        {
            size_t bundleSlash = path.rfind('/', bundlePos);
            if (bundleSlash != std::string::npos)
            {
                cachedRoot = path.substr(0, bundleSlash);
                return cachedRoot;
            }
        }

        return cachedRoot;
    }
};
