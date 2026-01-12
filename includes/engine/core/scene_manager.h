#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include <engine/core/scene.h>
#include <engine/core/resource_manager.h>
#include <engine/core/sound_manager.h>
#include <engine/physic/physic_world.h>
#include <engine/ecs/component.h>

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

    void ClearAllScenes();

private:
    friend class Scene;

    Scene &m_Scene;
    ResourceManager &m_Resources;
    SoundManager &m_SoundManager;
    PhysicsWorld &m_Physics;
    Application* m_App = nullptr; // [NEW]

    entt::entity currentEntity = entt::null;

    std::map<std::string, std::vector<entt::entity>> m_LoadedScenes;
};