#pragma once

#include <core/logic/config_loader.h>
#include <scene/logic/scene.h>
#include <scene/type/scene_record.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class IPhysicsWorld;
class ResourceManager;

namespace entt
{
enum class entity : uint32_t;
}

class SceneManager
{
public:
    SceneManager();
    void Initialize();

    void AddEntity(entt::entity entity, const std::string& sceneName);
    void RemoveEntity(entt::entity entity);

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
    bool HasPendingScene() const
    {
        return !m_PendingQueue.empty();
    }

    bool IsLoaded(const std::string& filePath) const;
    const SceneRecord* GetScene(const std::string& filePath) const;
    const SceneRecord* GetSceneByName(const std::string& name) const;
    const SceneRecord* GetSceneByOrder(int order) const;
    const std::vector<SceneRecord>& GetAllScenes() const
    {
        return m_LoadedScenes;
    }
    std::vector<const SceneRecord*> GetScenes() const;
    int GetSceneCount() const
    {
        return static_cast<int>(m_LoadedScenes.size());
    }

    void LogScene(const std::string& filePath) const;
    void LogAllScenes() const;

    void SetActiveScene(const std::string& name)
    {
        m_ActiveSceneName = name;
    }
    std::string GetActiveScene() const
    {
        return m_ActiveSceneName;
    }

    void SetSceneActive(const std::string& name, bool active, Scene& scene);
    void RebuildEntityRecords(Scene& scene);

    IPhysicsWorld* GetPhysicsWorld();

private:
    struct PendingOp
    {
        enum Type
        {
            Load,
            Unload,
            Change,
            Pop
        } type;
        std::string path;
        bool persistent = false;
    };

    void Internal_DestroySceneEntities(SceneRecord& rec);
    void Internal_UnloadOrphanedResources(const SceneRecord& rec);
    void Internal_UnloadRecord(SceneRecord& rec);
    void Internal_ReindexScenes();

    int m_NextLoadOrder = 0;
    std::string m_ActiveSceneName = "main";
    std::vector<SceneRecord> m_LoadedScenes;
    // SceneRecord keeps stable iteration order for serialization and teardown. This
    // companion lookup makes repeated membership checks O(1) while building large
    // procedural scenes instead of scanning the growing vector for every entity.
    std::unordered_map<std::string, std::unordered_set<entt::entity>> m_SceneEntityLookup;
    std::vector<PendingOp> m_PendingQueue;
};
