#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

class ResourceWatcher
{
public:
    using ReloadCallback = std::function<void(const std::string& name)>;

    ResourceWatcher();
    ~ResourceWatcher();

    void Watch(const std::string& name, const std::string& path, const std::string& type);
    void Watch(const std::string& name, const std::string& path, const std::string& type,
               const std::string& vsPath, const std::string& fsPath, const std::string& gsPath);
    
    void Update(float dt);
    
    void SetShaderReloadCallback(ReloadCallback cb) { m_OnShaderReload = cb; }
    void SetTextureReloadCallback(ReloadCallback cb) { m_OnTextureReload = cb; }

private:
    struct WatchEntry
    {
        std::string name;
        std::string filePath;
        std::filesystem::file_time_type lastWriteTime;
        std::string type;
        std::string vsPath;
        std::string fsPath;
        std::string gsPath;
    };

    std::vector<WatchEntry> m_Watchers;
    float m_HotReloadTimer;
    
    ReloadCallback m_OnShaderReload;
    ReloadCallback m_OnTextureReload;
};
