#include <state/state.h>
#include <app/application.h>

RenderSystem& State::GetRenderSystem() { return m_App->GetRenderSystem(); }
PhysicsSystem& State::GetPhysicsSystem() { return m_App->GetPhysicsSystem(); }
AudioSystem& State::GetAudioSystem() { return m_App->GetAudioSystem(); }
UIRenderSystem& State::GetUIRenderSystem() { return m_App->GetUIRenderSystem(); }
ScriptableSystem& State::GetScriptSystem() { return m_App->GetScriptSystem(); }
ParticleSystem& State::GetParticleSystem() { return m_App->GetParticleSystem(); }
SkyboxRenderSystem& State::GetSkyboxRenderSystem() { return m_App->GetSkyboxRenderSystem(); }
AnimationSystem& State::GetAnimationSystem() { return m_App->GetAnimationSystem(); }
VideoSystem& State::GetVideoSystem() { return m_App->GetVideoSystem(); }

SceneManager& State::GetSceneManager() { return m_App->GetSceneManager(); }
ResourceManager& State::GetResourceManager() { return m_App->GetResourceManager(); }
SoundPlayer& State::GetSoundPlayer() { return m_App->GetSoundPlayer(); }
IOHandler& State::GetIOHandler() { return m_App->GetIOHandler(); }
InputManager& State::GetInputManager() { return GetIOHandler().GetInputManager(); }
KeyboardManager& State::GetKeyboard() { return GetIOHandler().GetKeyboard(); }
MouseManager& State::GetMouse() { return GetIOHandler().GetMouse(); }

void State::LoadScene(const std::string& path) { GetSceneManager().LoadScene(path); }
void State::UnloadScene(const std::string& path) { GetSceneManager().UnloadScene(path); }
void State::ChangeScene(const std::string& path) { GetSceneManager().ChangeScene(path); }
void State::SetCursorMode(Input::CursorMode mode) { GetMouse().SetCursorMode(mode); }

void State::EnablePhysics(bool enable) { GetPhysicsSystem().SetEnabled(enable); }
void State::EnableRender(bool enable) { GetRenderSystem().SetEnabled(enable); }
void State::EnableAudio(bool enable) { GetAudioSystem().SetEnabled(enable); }

void State::EnableScript(bool enable) { GetScriptSystem().SetEnabled(enable); }
void State::EnableAnimation(bool enable) { GetAnimationSystem().SetEnabled(enable); }
void State::EnableVideo(bool enable) { GetVideoSystem().SetEnabled(enable); }
void State::EnableUIRender(bool enable) { GetUIRenderSystem().SetEnabled(enable); }
void State::EnableParticle(bool enable) { GetParticleSystem().SetEnabled(enable); }
void State::EnableSkybox(bool enable) { GetSkyboxRenderSystem().SetEnabled(enable); }

void State::EnableLogic(bool enable) {
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
}
