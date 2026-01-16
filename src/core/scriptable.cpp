#include <core/scriptable.h>
#include <core/application.h>

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

SoundManager& Scriptable::GetSoundManager()
{
    return m_App->GetSoundManager();
}

ResourceManager& Scriptable::GetResourceManager()
{
    return m_App->GetResourceManager();
}

AppHandler& Scriptable::GetAppHandler()
{
    return m_App->GetAppHandler();
}

SceneManager& Scriptable::GetSceneManager()
{
    return m_App->GetSceneManager();
}

InputManager& Scriptable::GetInputManager() { return GetAppHandler().GetInputManager(); }
KeyboardManager& Scriptable::GetKeyboard() { return GetAppHandler().GetKeyboard(); }
MouseManager& Scriptable::GetMouse() { return GetAppHandler().GetMouse(); }
