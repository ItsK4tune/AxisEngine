#include <resource/resource_watcher.h>
#include <iostream>

ResourceWatcher::ResourceWatcher()
    : m_HotReloadTimer(0.0f)
{
}

ResourceWatcher::~ResourceWatcher()
{
}

void ResourceWatcher::Watch(const std::string& name, const std::string& path, const std::string& type)
{
    bool alreadyWatched = false;
    for (auto& w : m_Watchers)
    {
        if (w.name == name && w.type == type)
        {
            alreadyWatched = true;
            break;
        }
    }
    
    if (!alreadyWatched)
    {
        WatchEntry entry;
        entry.name = name;
        entry.filePath = path;
        entry.type = type;
        
        try
        {
            entry.lastWriteTime = std::filesystem::last_write_time(path);
        }
        catch (...)
        {
        }
        
        m_Watchers.push_back(entry);
    }
}

void ResourceWatcher::Watch(const std::string& name, const std::string& path, const std::string& type,
                           const std::string& vsPath, const std::string& fsPath, const std::string& gsPath)
{
    WatchEntry entry;
    entry.name = name;
    entry.filePath = path;
    entry.type = type;
    entry.vsPath = vsPath;
    entry.fsPath = fsPath;
    entry.gsPath = gsPath;
    
    try
    {
        entry.lastWriteTime = std::filesystem::last_write_time(path);
    }
    catch (...)
    {
    }
    
    m_Watchers.push_back(entry);
}

void ResourceWatcher::Update(float dt)
{
    m_HotReloadTimer += dt;
    
    if (m_HotReloadTimer > 1.0f)
    {
        for (auto& watcher : m_Watchers)
        {
            try
            {
                auto currentWriteTime = std::filesystem::last_write_time(watcher.filePath);
                if (currentWriteTime > watcher.lastWriteTime)
                {
                    watcher.lastWriteTime = currentWriteTime;
                    std::cout << "[HotReload] Detected change in: " << watcher.filePath << std::endl;
                    
                    if (watcher.type == "SHADER")
                    {
                        if (m_OnShaderReload)
                            m_OnShaderReload(watcher.name);
                    }
                    else if (watcher.type == "TEXTURE")
                    {
                        if (m_OnTextureReload)
                            m_OnTextureReload(watcher.name);
                    }
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
            }
        }
        
        m_HotReloadTimer = 0.0f;
    }
}
