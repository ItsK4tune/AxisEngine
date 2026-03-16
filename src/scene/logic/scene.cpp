#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <physics/interface/i_physics_world.h>
#include <script/logic/scriptable.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/logger.h>
#include <vector>

Scene::Scene()
{
    registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);
    registry.on_construct<HierarchyComponent>().connect<&Scene::OnHierarchyChanged>(this);
    registry.on_destroy<HierarchyComponent>().connect<&Scene::OnHierarchyChanged>(this);
    registry.on_update<HierarchyComponent>().connect<&Scene::OnHierarchyChanged>(this);
}

Scene::~Scene()
{
    registry.on_destroy<ScriptComponent>().disconnect<&Scene::OnScriptComponentDestroyed>(this);
    registry.on_construct<HierarchyComponent>().disconnect<&Scene::OnHierarchyChanged>(this);
    registry.on_destroy<HierarchyComponent>().disconnect<&Scene::OnHierarchyChanged>(this);
    registry.on_update<HierarchyComponent>().disconnect<&Scene::OnHierarchyChanged>(this);
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
                sc->instance.reset();
            } catch (...) {
                LOGGER_ERROR("Scene") << "OnScriptComponentDestroyed: CRASH during script cleanup for entity " << (uint32_t)entity;
            }
        }
    }
}

void Scene::OnHierarchyChanged(entt::registry &reg, entt::entity entity)
{
    isLinearTransformsDirty = true;
}



#include <deque>

void Scene::InitializeManagers()
{
    m_Octree = std::make_unique<Octree>(AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)));
}

void Scene::ShutdownManagers()
{
    m_Octree.reset();
}

void Scene::RebuildLinearTransforms()
{
    if (!isLinearTransformsDirty) return;

    linearTransforms.clear();
    std::vector<entt::entity> roots;
    auto transformView = registry.view<WorldTransformComponent>();
    
    for (auto entity : transformView)
    {
        auto* hierarchy = registry.try_get<HierarchyComponent>(entity);
        if (!hierarchy || hierarchy->parent == entt::null)
        {
            roots.push_back(entity);
        }
    }

    std::deque<entt::entity> queue(roots.begin(), roots.end());
    while (!queue.empty())
    {
        entt::entity current = queue.front();
        queue.pop_front();

        linearTransforms.push_back(current);

        auto* hierarchy = registry.try_get<HierarchyComponent>(current);
        if (hierarchy)
        {
            for (auto child : hierarchy->children)
            {
                if (registry.valid(child) && registry.all_of<WorldTransformComponent>(child))
                {
                    queue.push_back(child);
                }
            }
        }
    }

    isLinearTransformsDirty = false;
}
