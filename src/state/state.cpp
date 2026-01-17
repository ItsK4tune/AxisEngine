#include <state/state.h>
#include <app/application.h>

RenderSystem& State::GetRenderSystem() { return m_App->GetRenderSystem(); }
PhysicsSystem& State::GetPhysicsSystem() { return m_App->GetPhysicsSystem(); }
AudioSystem& State::GetAudioSystem() { return m_App->GetAudioSystem(); }
UIRenderSystem& State::GetUIRenderSystem() { return m_App->GetUIRenderSystem(); }
UIInteractSystem& State::GetUIInteractSystem() { return m_App->GetUIInteractSystem(); }
ScriptableSystem& State::GetScriptSystem() { return m_App->GetScriptSystem(); }
ParticleSystem& State::GetParticleSystem() { return m_App->GetParticleSystem(); }
SkyboxRenderSystem& State::GetSkyboxRenderSystem() { return m_App->GetSkyboxRenderSystem(); }
AnimationSystem& State::GetAnimationSystem() { return m_App->GetAnimationSystem(); }
VideoSystem& State::GetVideoSystem() { return m_App->GetVideoSystem(); }

SceneManager& State::GetSceneManager() { return m_App->GetSceneManager(); }
ResourceManager& State::GetResourceManager() { return m_App->GetResourceManager(); }
SoundManager& State::GetSoundManager() { return m_App->GetSoundManager(); }
AppHandler& State::GetAppHandler() { return m_App->GetAppHandler(); }
InputManager& State::GetInputManager() { return GetAppHandler().GetInputManager(); }
KeyboardManager& State::GetKeyboard() { return GetAppHandler().GetKeyboard(); }
MouseManager& State::GetMouse() { return GetAppHandler().GetMouse(); }

void State::LoadScene(const std::string& path) { GetSceneManager().LoadScene(path); }
void State::UnloadScene(const std::string& path) { GetSceneManager().UnloadScene(path); }
void State::ChangeScene(const std::string& path) { GetSceneManager().ChangeScene(path); }
void State::SetCursorMode(CursorMode mode) { GetMouse().SetCursorMode(mode); }

void State::EnablePhysics(bool enable) { GetPhysicsSystem().SetEnabled(enable); }
void State::EnableRender(bool enable) { GetRenderSystem().SetEnabled(enable); }
void State::EnableAudio(bool enable) { GetAudioSystem().SetEnabled(enable); }

void State::EnableScript(bool enable) { GetScriptSystem().SetEnabled(enable); }
void State::EnableAnimation(bool enable) { GetAnimationSystem().SetEnabled(enable); }
void State::EnableVideo(bool enable) { GetVideoSystem().SetEnabled(enable); }
void State::EnableUIInteract(bool enable) { GetUIInteractSystem().SetEnabled(enable); }
void State::EnableUIRender(bool enable) { GetUIRenderSystem().SetEnabled(enable); }
void State::EnableParticle(bool enable) { GetParticleSystem().SetEnabled(enable); }
void State::EnableSkybox(bool enable) { GetSkyboxRenderSystem().SetEnabled(enable); }

void State::EnableLogic(bool enable) { 
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableUIInteract(enable);
    EnableParticle(enable);
}
