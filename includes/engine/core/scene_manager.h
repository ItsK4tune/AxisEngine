#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include <engine/core/scene.h>
#include <engine/core/resource_manager.h>
#include <engine/core/sound_manager.h>
#include <engine/physic/physic_world.h>

class SceneManager {
public:
    SceneManager(Scene& scene, ResourceManager& res, PhysicsWorld& phys, SoundManager& sound);

    void LoadScene(const std::string& filePath);
    void UnloadScene(const std::string& filePath);
    
    void ClearAllScenes();

private:
    Scene& m_Scene;
    ResourceManager& m_Resources;
    SoundManager& m_SoundManager;
    PhysicsWorld& m_Physics;

    entt::entity currentEntity = entt::null;

    std::map<std::string, std::vector<entt::entity>> m_LoadedScenes;
    
    void DestroyEntity(entt::entity entity);
};