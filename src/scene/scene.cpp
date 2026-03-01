#include <scene/scene.h>
#include <interface/physics/i_physics_world.h>
#include <scene/scene_manager.h>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <algorithm>
#include <utils/logger.h>

Scene::Scene()
{
    registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);
}

Scene::~Scene()
{
    registry.on_destroy<ScriptComponent>().disconnect<&Scene::OnScriptComponentDestroyed>(this);
}

void Scene::OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity)
{
    if (auto sc = reg.try_get<ScriptComponent>(entity))
    {
        if (sc->instance)
        {
            try {
                if (sc->DestroyScript)
                    sc->DestroyScript(sc);
                sc->instance = nullptr;
            } catch (...) {
                LOGGER_ERROR("Scene") << "OnScriptComponentDestroyed: CRASH during script cleanup for entity " << (uint32_t)entity;
            }
        }
    }
}




void Scene::InitializeManagers()
{
    m_Octree = std::make_unique<Octree>(AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)));
}

void Scene::ShutdownManagers()
{
    m_Octree.reset();
}
