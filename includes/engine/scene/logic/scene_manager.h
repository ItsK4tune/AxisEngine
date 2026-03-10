#pragma once

#include <audio/logic/sound_player.h>
#include <core/logic/config_loader.h>
#include <core/unit/engine_context.h>
#include <ecs/unit/core_components.h>
#include <memory>
#include <physics/interface/i_physics_world.h>
#include <resource/manager/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/type/scene_record.h>
#include <string>
#include <vector>

class SceneManager
{
public:
    SceneManager();
    void Initialize(EngineContext ctx, std::function<void(const AppConfig&)> applyConfigFn = nullptr);

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

    IPhysicsWorld* GetPhysicsWorld() { return m_Physics; }

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

    Scene* m_Scene = nullptr;
    ResourceManager* m_Resources = nullptr;
    SoundPlayer* m_SoundPlayer = nullptr;
    IPhysicsWorld* m_Physics = nullptr;
    EngineContext m_Ctx;
    std::function<void(const AppConfig&)> m_ApplyConfigFn;

    int m_nextLoadOrder = 0;
    std::vector<SceneRecord> m_LoadedScenes;
    std::vector<PendingOp>   m_pendingQueue;
};
