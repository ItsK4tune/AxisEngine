#include <core/logic/config_loader.h>
#include <platform/logic/io_handler.h>
#include <core/logic/runtime_core.h>
#include <core/logic/system_manager.h>
#include <audio/logic/audio_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <platform/logic/input_manager.h>
#include <physics/logic/collision_matrix.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene_manager.h>
#include <script/logic/scriptable.h>



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
    CollisionMatrix::Instance().IgnoreTagCollision(tag1, tag2);
}

void Scriptable::IgnoreNameCollision(const std::string& name1, const std::string& name2)
{
    CollisionMatrix::Instance().IgnoreNameCollision(name1, name2);
}
