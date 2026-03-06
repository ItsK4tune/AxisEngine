#include <app/config_loader.h>
#include <core/engine_context.h>
#include <window/io_handler.h>
#include <core/runtime_core.h>
#include <core/system_manager.h>
#include <audio/sound_player.h>
#include <ecs/components/info_component.h>
#include <ecs/components/physics_components.h>
#include <ecs/systems/audio_system.h>
#include <ecs/systems/physics_system.h>
#include <ecs/systems/render_system.h>
#include <input/input_loader.h>
#include <input/input_manager.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <physics/collision_matrix.h>
#include <resource/resource_manager.h>
#include <scene/scene_manager.h>
#include <script/scriptable.h>



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
