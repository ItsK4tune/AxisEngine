#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h>
#include <scene/octree.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

class SceneManager;

struct Scene
{
    Scene();
    ~Scene();

    entt::registry registry;
    Octree* GetOctree() { return m_Octree.get(); }

    void OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity);

    void InitializeManagers();
    void ShutdownManagers();

private:
    std::unique_ptr<Octree> m_Octree;

    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;
};
