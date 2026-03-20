#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <physics/interface/i_physics_world.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/logger.h>
#include <vector>

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::InitializeManagers()
{
    m_Octree = std::make_unique<Octree>(AABB(glm::vec3(-1000.0f), glm::vec3(1000.0f)));
}

void Scene::ShutdownManagers()
{
    m_Octree.reset();
}
