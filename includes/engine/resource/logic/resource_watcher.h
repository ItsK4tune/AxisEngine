#pragma once

#include <atomic>
#include <condition_variable>
#include <core/logic/event_types.h>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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
    std::thread m_WatcherThread;
    std::condition_variable m_StopCV;
    std::mutex m_StopMutex;

    void WatcherLoop();
};