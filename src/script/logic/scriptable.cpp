#include <script/logic/scriptable.h>
#include <scene/logic/scene.h>
#include <ecs/unit/script_component.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <core/logic/service_locator.h>
#include <physics/logic/collision_matrix.h>

// Note: Template implementations for GetComponent/HasComponent/GetScript 
// should technically be in the header if they are truly generic, 
// OR explicitly instantiated in the cpp for known types.
// Since entt::registry::get is a template, we'll keep them in a separate header 
// or move them back if they cause linker errors. 
// For now, I'll move them back to the header but keep them out of the main block 
// or use a .tpp file to keep the header "clean".

// Wait, the user wants "chặt" (tight) headers. 
// Let's keep the templates in the header but minimize OTHER includes.



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
