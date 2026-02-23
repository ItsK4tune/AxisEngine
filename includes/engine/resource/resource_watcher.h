#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <event/resource_events.h>

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
    std::vector<ResourceReloadEvent> m_PendingReloads;
    std::mutex m_Mutex;
    std::atomic<bool> m_Running;

    void WatcherLoop();
};
