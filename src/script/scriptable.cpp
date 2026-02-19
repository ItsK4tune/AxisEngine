#include <script/scriptable.h>
#include <app/application.h>
#include <input/input_manager.h>

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
