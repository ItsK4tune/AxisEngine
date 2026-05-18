#include <script/logic/scriptable.h>
#include <scene/logic/scene.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <core/logic/service_locator.h>
#include <physics/logic/collision_matrix.h>
#include <ecs/logic/entity_manager.h>














bool Scriptable::CompareTag(entt::entity entity, const std::string& tag) const
{
    if (GetScene().registry.valid(entity) && GetScene().registry.all_of<InfoComponent>(entity))
    {
        return GetScene().registry.get<InfoComponent>(entity).tag == tag;
    }
    return false;
}

bool Scriptable::CompareName(entt::entity entity, const std::string& name) const
{
    if (GetScene().registry.valid(entity) && GetScene().registry.all_of<InfoComponent>(entity))
    {
        return GetScene().registry.get<InfoComponent>(entity).name == name;
    }
    return false;
}

std::string Scriptable::GetTag(entt::entity entity) const
{
    if (GetScene().registry.valid(entity) && GetScene().registry.all_of<InfoComponent>(entity))
    {
        return GetScene().registry.get<InfoComponent>(entity).tag;
    }
    return "";
}

std::string Scriptable::GetName(entt::entity entity) const
{
    if (GetScene().registry.valid(entity) && GetScene().registry.all_of<InfoComponent>(entity))
    {
        return GetScene().registry.get<InfoComponent>(entity).name;
    }
    return "";
}

void Scriptable::SetCollisionEnabled(bool enabled)
{
    if (GetScene().registry.valid(m_Entity) && GetScene().registry.all_of<RigidBodyComponent>(m_Entity))
    {
        GetScene().registry.get<RigidBodyComponent>(m_Entity).isCollisionEnabled = enabled;
    }
}

bool Scriptable::IsCollisionEnabled() const
{
    if (GetScene().registry.valid(m_Entity) && GetScene().registry.all_of<RigidBodyComponent>(m_Entity))
    {
        return GetScene().registry.get<RigidBodyComponent>(m_Entity).isCollisionEnabled;
    }
    return false;
}

void Scriptable::IgnoreTagCollision(const std::string& tag1, const std::string& tag2)
{
    auto matrix = ServiceLocator::Instance().Resolve<CollisionMatrix>();
    if (matrix) matrix->IgnoreTagCollision(tag1, tag2);
}

void Scriptable::IgnoreNameCollision(const std::string& name1, const std::string& name2)
{
    auto matrix = ServiceLocator::Instance().Resolve<CollisionMatrix>();
    if (matrix) matrix->IgnoreNameCollision(name1, name2);
}

entt::entity Scriptable::Spawn(const std::string& name, const std::string& tag)
{
    return EntityManager::CreateEntity(GetScene(), name, tag);
}

entt::entity Scriptable::Spawn(const std::string& name, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
{
    return EntityManager::CreateEntityWithTransform(GetScene(), name, position, rotation, scale);
}

void Scriptable::Destroy(entt::entity entity)
{
    EntityManager::Destroy(GetScene(), entity);
}

void Scriptable::Invoke(std::function<void()> callback, float delay)
{
    if (callback)
    {
        m_PendingInvokes.push_back({callback, delay});
    }
}

void Scriptable::UpdateInvokes(float dt)
{
    for (auto it = m_PendingInvokes.begin(); it != m_PendingInvokes.end(); )
    {
        it->delay -= dt;
        if (it->delay <= 0.0f)
        {
            try {
                if (it->callback) it->callback();
            } catch (const std::exception& e) {
                LOGGER_ERROR("Scriptable") << "Invoke callback CRASH on entity " << (uint32_t)m_Entity << ": " << e.what();
            } catch (...) {
                LOGGER_ERROR("Scriptable") << "Invoke callback UNKNOWN CRASH on entity " << (uint32_t)m_Entity;
            }
            it = m_PendingInvokes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
