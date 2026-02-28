#pragma once

#include <string>
#include <vector>
#include <memory>

#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <audio/sound_player.h>
#include <interface/physics/i_physics_world.h>
#include <ecs/component.h>
#include <app/config_loader.h>
#include <scene/scene_types.h>

class Application;

class SceneManager
{
public:
    SceneManager(Scene& scene, ResourceManager& res, IPhysicsWorld& phys, SoundPlayer& sound, Application* app);

    // Legacy helper used by SceneSerializer
    void AddEntity(entt::entity entity, const std::string& sceneName);

    // Load / Unload
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

    // Queue (end-of-frame)
    void QueueLoadScene(const std::string& path, bool persistent = false);
    void QueueUnloadScene(const std::string& path);
    void QueueChangeScene(const std::string& path);
    void QueuePopScene();
    void UpdatePendingScene();
    bool HasPendingScene() const { return !m_pendingQueue.empty(); }

    // Query
    bool IsLoaded(const std::string& filePath) const;
    const SceneRecord* GetScene(const std::string& filePath) const;
    const SceneRecord* GetSceneByName(const std::string& name) const;
    const SceneRecord* GetSceneByOrder(int order) const;
    const std::vector<SceneRecord>& GetAllScenes() const { return m_LoadedScenes; }
    std::vector<const SceneRecord*> GetScenes() const;
    int GetSceneCount() const { return static_cast<int>(m_LoadedScenes.size()); }

    // Log
    void LogScene(const std::string& filePath) const;
    void LogAllScenes() const;

    IPhysicsWorld& GetPhysicsWorld() { return m_Physics; }

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

    Scene&           m_Scene;
    ResourceManager& m_Resources;
    SoundPlayer&     m_SoundPlayer;
    IPhysicsWorld&   m_Physics;
    Application*     m_App = nullptr;

    int m_nextLoadOrder = 0;
    std::vector<SceneRecord> m_LoadedScenes;
    std::vector<PendingOp>   m_pendingQueue;
};
