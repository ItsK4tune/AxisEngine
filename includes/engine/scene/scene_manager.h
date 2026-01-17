#pragma once

#include <string>
#include <string>
#include <vector>
#include <unordered_map>

#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <audio/sound_manager.h>
#include <physic/physic_world.h>
#include <ecs/component.h>

class Application; // Forward declaration

class SceneManager
{
public:
    SceneManager(Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound, Application* app);

    void AddEntity(entt::entity entity, const std::string& sceneName) {
        m_LoadedScenes[sceneName].push_back(entity);
    }

    void LoadScene(const std::string &filePath);
    void UnloadScene(const std::string &filePath);
    void ChangeScene(const std::string &filePath);

    void ClearAllScenes();

    PhysicsWorld& GetPhysicsWorld() { return m_Physics; }

    // Deferred Loading
    void QueueLoadScene(const std::string& path);
    void UpdatePendingScene();
    bool HasPendingScene() const { return m_isPending; }

private:
    Scene &m_Scene;
    ResourceManager &m_Resources;
    SoundManager &m_SoundManager;
    PhysicsWorld &m_Physics;
    Application* m_App = nullptr;

    std::map<std::string, std::vector<entt::entity>> m_LoadedScenes;
    
    std::string m_pendingPath = "";
    bool m_isPending = false;
};