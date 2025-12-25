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

class SceneManager
{
public:
    SceneManager(Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound);

    template <typename T>
    void RegisterScript(const std::string &name)
    {
        m_ScriptRegistry[name] = [](Scene &scene, entt::entity entity)
        {
            auto &sc = scene.registry.emplace_or_replace<ScriptComponent>(entity);
            sc.Bind<T>();
        };
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

    entt::entity currentEntity = entt::null;

    std::map<std::string, std::vector<entt::entity>> m_LoadedScenes;
    std::unordered_map<std::string, std::function<void(Scene &, entt::entity)>> m_ScriptRegistry;
};