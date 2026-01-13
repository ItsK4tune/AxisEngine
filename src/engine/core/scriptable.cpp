#include <engine/core/scriptable.h>
#include <engine/core/application.h>

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
