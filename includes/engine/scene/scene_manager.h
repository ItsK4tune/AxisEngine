#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <audio/sound_player.h>
#include <interface/physics/i_physics_world.h>
#include <ecs/component.h>

class Application;

class SceneManager
{
public:
    SceneManager(Scene &scene, ResourceManager &res, IPhysicsWorld &phys, SoundPlayer &sound, Application* app);

    void AddEntity(entt::entity entity, const std::string &sceneName)
    {
        m_LoadedScenes[sceneName].push_back(entity);
    }

    void LoadScene(const std::string &filePath);
    void UnloadScene(const std::string &filePath);
    void ChangeScene(const std::string &filePath);

    void ClearAllScenes();

    IPhysicsWorld &GetPhysicsWorld() { return m_Physics; }

    void QueueLoadScene(const std::string &path);
    void UpdatePendingScene();
    bool HasPendingScene() const { return m_isPending; }

private:
    Scene &m_Scene;
    ResourceManager &m_Resources;
    SoundPlayer &m_SoundPlayer;
    IPhysicsWorld &m_Physics;
    Application* m_App = nullptr;

    std::map<std::string, std::vector<entt::entity>> m_LoadedScenes;

    std::string m_pendingPath = "";
    bool m_isPending = false;
};
