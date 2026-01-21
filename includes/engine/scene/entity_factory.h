#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h>
#include <string>
#include <glm/glm.hpp>

class Scene;

class EntityFactory
{
public:
    EntityFactory(Scene &scene);
    ~EntityFactory();

    entt::entity CreateEntity(const std::string &name = "unnamed", const std::string &tag = "default");
    entt::entity CreateEntityWithTransform(const std::string &name, const glm::vec3 &position, const glm::vec3 &rotation = glm::vec3(0.0f), const glm::vec3 &scale = glm::vec3(1.0f));

    entt::entity CreateEmptyEntity(const std::string &name = "Empty");
    entt::entity CreateCube(const std::string &name, const glm::vec3 &position);
    entt::entity CreateSphere(const std::string &name, const glm::vec3 &position);
    entt::entity CreatePlane(const std::string &name, const glm::vec3 &position);

    void SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform = true);
    void AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform = true);

    void DestroyEntity(entt::entity entity);
    void DestroyEntityWithChildren(entt::entity entity);

private:
    Scene &m_Scene;

    void DestroyEntityRecursive(entt::entity entity);
};
