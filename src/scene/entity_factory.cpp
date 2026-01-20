#include <scene/entity_factory.h>
#include <scene/scene.h>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

EntityFactory::EntityFactory(Scene& scene)
    : m_Scene(scene)
{
}

EntityFactory::~EntityFactory()
{
}

entt::entity EntityFactory::CreateEntity(const std::string& name, const std::string& tag)
{
    entt::entity entity = m_Scene.createEntity();
    m_Scene.registry.emplace<InfoComponent>(entity, name, tag);
    return entity;
}

entt::entity EntityFactory::CreateEntityWithTransform(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{
    entt::entity entity = CreateEntity(name);
    
    auto& transform = m_Scene.registry.get<TransformComponent>(entity);
    transform.position = position;
    transform.rotation = glm::quat(rotation);
    transform.scale = scale;
    
    return entity;
}

entt::entity EntityFactory::CreateEmptyEntity(const std::string& name)
{
    return CreateEntityWithTransform(name, glm::vec3(0.0f));
}

entt::entity EntityFactory::CreateCube(const std::string& name, const glm::vec3& position)
{
    entt::entity entity = CreateEntityWithTransform(name, position);
    return entity;
}

entt::entity EntityFactory::CreateSphere(const std::string& name, const glm::vec3& position)
{
    entt::entity entity = CreateEntityWithTransform(name, position);
    return entity;
}

entt::entity EntityFactory::CreatePlane(const std::string& name, const glm::vec3& position)
{
    entt::entity entity = CreateEntityWithTransform(name, position);
    return entity;
}

void EntityFactory::SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform)
{
    if (!m_Scene.registry.valid(child) || !m_Scene.registry.valid(parent))
    {
        std::cerr << "[EntityFactory] Invalid child or parent entity" << std::endl;
        return;
    }
    
    if (!m_Scene.registry.all_of<TransformComponent>(child) || !m_Scene.registry.all_of<TransformComponent>(parent))
    {
        std::cerr << "[EntityFactory] Child or parent missing TransformComponent" << std::endl;
        return;
    }
    
    auto& childTransform = m_Scene.registry.get<TransformComponent>(child);
    childTransform.SetParent(child, parent, m_Scene.registry, keepWorldTransform);
}

void EntityFactory::AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform)
{
    SetParent(child, parent, keepWorldTransform);
}

void EntityFactory::DestroyEntity(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;
    
    if (m_Scene.registry.all_of<TransformComponent>(entity))
    {
        auto& transform = m_Scene.registry.get<TransformComponent>(entity);
        if (m_Scene.registry.valid(transform.parent))
        {
            transform.SetParent(entity, entt::null, m_Scene.registry, false);
        }
    }
    
    m_Scene.registry.destroy(entity);
}

void EntityFactory::DestroyEntityWithChildren(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;
    
    DestroyEntityRecursive(entity);
}

void EntityFactory::DestroyEntityRecursive(entt::entity entity)
{
    if (!m_Scene.registry.valid(entity))
        return;
    
    // Get children before destroying
    std::vector<entt::entity> children;
    if (m_Scene.registry.all_of<TransformComponent>(entity))
    {
        const auto& transform = m_Scene.registry.get<TransformComponent>(entity);
        children = transform.children;
    }
    
    // Recursively destroy children first
    for (auto child : children)
    {
        DestroyEntityRecursive(child);
    }
    
    // Destroy this entity
    DestroyEntity(entity);
}
