#pragma once

#include <scene/logic/octree.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>

class SceneManager;

#define GLM_ENABLE_EXPERIMENTAL

struct Scene
{
    Scene();
    ~Scene();

    entt::registry registry;
    Octree* GetOctree()
    {
        return m_Octree.get();
    }

    void InitializeManagers();
    void ShutdownManagers();

private:
    std::unique_ptr<Octree> m_Octree;

    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;
};
