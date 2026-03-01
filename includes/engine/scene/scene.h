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

    entt::entity CreateEntity(const std::string &name = "unnamed", const std::string &tag = "default");
    entt::entity CreateEntityWithTransform(const std::string &name, const glm::vec3 &position, const glm::vec3 &rotation = glm::vec3(0.0f), const glm::vec3 &scale = glm::vec3(1.0f));

    entt::entity CreateEmptyEntity(const std::string &name = "Empty");
    entt::entity CreateCube(const std::string &name, const glm::vec3 &position);
    entt::entity CreateSphere(const std::string &name, const glm::vec3 &position);
    entt::entity CreatePlane(const std::string &name, const glm::vec3 &position);

    void SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform = true);
    void AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform = true);

    void DestroyEntity(entt::entity entity, SceneManager *manager = nullptr);
    void DestroyEntityWithChildren(entt::entity entity, SceneManager *manager = nullptr);

    entt::entity GetActiveCamera();
    void SetActiveCamera(entt::entity entity);

    entt::entity GetActiveSkybox() const;
    void SetActiveSkybox(entt::entity entity);

    void OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity);

    Octree* GetOctree() { return m_Octree.get(); }

    void InitializeManagers();
    void ShutdownManagers();

private:
    std::unique_ptr<Octree> m_Octree;

    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;
};
