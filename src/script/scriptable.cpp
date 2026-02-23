#include <script/scriptable.h>
#include <app/application.h>
#include <input/input_manager.h>
#include <ecs/components/info_component.h>
#include <ecs/components/physics_components.h>
#include <physic/collision_matrix.h>

bool Scriptable::GetAction(const std::string& name)
{
    return m_App->GetInputManager().GetAction(name);
}

bool Scriptable::GetActionDown(const std::string& name)
{
    return m_App->GetInputManager().GetActionDown(name);
}

bool Scriptable::GetActionUp(const std::string& name)
{
    return m_App->GetInputManager().GetActionUp(name);
}

void Scriptable::LoadScene(const std::string& path)
{
    m_App->GetSceneManager().QueueLoadScene(path);
}

void Scriptable::SetTimeScale(float scale)
{
    m_App->SetTimeScale(scale);
}

float Scriptable::GetTimeScale() const
{
    return m_App->GetTimeScale();
}

float Scriptable::GetRealDeltaTime() const
{
    return m_App->GetRealDeltaTime();
}

SoundPlayer& Scriptable::GetSoundPlayer()
{
    return m_App->GetSoundPlayer();
}

ResourceManager& Scriptable::GetResourceManager()
{
    return m_App->GetResourceManager();
}

IOHandler& Scriptable::GetIOHandler()
{
    return m_App->GetIOHandler();
}

SceneManager& Scriptable::GetSceneManager()
{
    return m_App->GetSceneManager();
}

InputManager& Scriptable::GetInputManager() { return GetIOHandler().GetInputManager(); }
KeyboardManager& Scriptable::GetKeyboard() { return GetIOHandler().GetKeyboard(); }
MouseManager& Scriptable::GetMouse() { return GetIOHandler().GetMouse(); }

bool Scriptable::CompareTag(entt::entity entity, const std::string& tag) const
{
    if (m_Scene->registry.valid(entity) && m_Scene->registry.all_of<InfoComponent>(entity))
    {
        return m_Scene->registry.get<InfoComponent>(entity).tag == tag;
    }
    return false;
}

bool Scriptable::CompareName(entt::entity entity, const std::string& name) const
{
    if (m_Scene->registry.valid(entity) && m_Scene->registry.all_of<InfoComponent>(entity))
    {
        return m_Scene->registry.get<InfoComponent>(entity).name == name;
    }
    return false;
}

std::string Scriptable::GetTag(entt::entity entity) const
{
    if (m_Scene->registry.valid(entity) && m_Scene->registry.all_of<InfoComponent>(entity))
    {
        return m_Scene->registry.get<InfoComponent>(entity).tag;
    }
    return "";
}

std::string Scriptable::GetName(entt::entity entity) const
{
    if (m_Scene->registry.valid(entity) && m_Scene->registry.all_of<InfoComponent>(entity))
    {
        return m_Scene->registry.get<InfoComponent>(entity).name;
    }
    return "";
}

void Scriptable::SetCollisionEnabled(bool enabled)
{
    if (m_Scene->registry.valid(m_Entity) && m_Scene->registry.all_of<RigidBodyComponent>(m_Entity))
    {
        m_Scene->registry.get<RigidBodyComponent>(m_Entity).isCollisionEnabled = enabled;
    }
}

bool Scriptable::IsCollisionEnabled() const
{
    if (m_Scene->registry.valid(m_Entity) && m_Scene->registry.all_of<RigidBodyComponent>(m_Entity))
    {
        return m_Scene->registry.get<RigidBodyComponent>(m_Entity).isCollisionEnabled;
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
