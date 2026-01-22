#pragma once

#include <string>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

class FileSystem
{
public:
    static std::string getPath(const std::string &path)
    {
        std::string root = getRoot();
        
        if (!root.empty() && path.find(root) == 0)
            return path;
            
        if (!root.empty())
            return root + "/" + path;
            
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
        
#ifdef _WIN32
        char exePath[MAX_PATH];
        DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            std::string path(exePath);
            std::replace(path.begin(), path.end(), '\\', '/');
            
            size_t binPos = path.rfind("/bin/");
            if (binPos != std::string::npos)
            {
                cachedRoot = path.substr(0, binPos);
                return cachedRoot;
            }
        }
#else
        char exePath[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
        if (len != -1)
        {
            exePath[len] = '\0';
            std::string path(exePath);
            
            size_t binPos = path.rfind("/bin/");
            if (binPos != std::string::npos)
            {
                cachedRoot = path.substr(0, binPos);
                return cachedRoot;
            }
        }
#endif
        return cachedRoot;
    }
};