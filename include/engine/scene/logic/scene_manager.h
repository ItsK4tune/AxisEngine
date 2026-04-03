#pragma once

#include <core/logic/config_loader.h>
#include <memory>
#include <scene/logic/scene.h>
#include <scene/type/scene_record.h>
#include <string>
#include <vector>

class IPhysicsWorld;
class ResourceManager;

namespace entt { enum class entity : uint32_t; }

class SceneManager
{
public:
    SceneManager();
    void Initialize();

    void AddEntity(entt::entity entity, const std::string& sceneName);

    void LoadScene(const std::string& filePath, bool persistent = false);
    void UnloadScene(const std::string& filePath);
    void UnloadScene(const SceneRecord* rec);
    void UnloadSceneByName(const std::string& name);
    void UnloadSceneByOrder(int order);
    void PopScene();
    void ChangeScene(const std::string& filePath);
    void ClearAllScenes();
    void ClearAllIncludingPersistent();
    void Shutdown();

    void QueueLoadScene(const std::string& path, bool persistent = false);
    void QueueUnloadScene(const std::string& path);
    void QueueChangeScene(const std::string& path);
    void QueuePopScene();
    void UpdatePendingScene();
    bool HasPendingScene() const { return !m_pendingQueue.empty(); }

    bool IsLoaded(const std::string& filePath) const;
    const SceneRecord* GetScene(const std::string& filePath) const;
    const SceneRecord* GetSceneByName(const std::string& name) const;
    const SceneRecord* GetSceneByOrder(int order) const;
    const std::vector<SceneRecord>& GetAllScenes() const { return m_LoadedScenes; }
    std::vector<const SceneRecord*> GetScenes() const;
    int GetSceneCount() const { return static_cast<int>(m_LoadedScenes.size()); }

    void LogScene(const std::string& filePath) const;
    void LogAllScenes() const;

    IPhysicsWorld* GetPhysicsWorld();

private:
    struct PendingOp
    {
        enum Type { Load, Unload, Change, Pop } type;
        std::string path;
        bool persistent = false;
    };

    void _DestroySceneEntities(SceneRecord& rec);
    void _UnloadOrphanedResources(const SceneRecord& rec);
    void _RollbackConfig(const SceneRecord& removed);
    void _UnloadRecord(SceneRecord& rec);
    void _ReindexScenes();

    int m_nextLoadOrder = 0;
    std::vector<SceneRecord> m_LoadedScenes;
    std::vector<PendingOp>   m_pendingQueue;
};
