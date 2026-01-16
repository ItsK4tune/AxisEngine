#include <core/state.h>
#include <core/application.h>

RenderSystem& State::GetRenderSystem() { return m_App->GetRenderSystem(); }
PhysicsSystem& State::GetPhysicsSystem() { return m_App->GetPhysicsSystem(); }
AudioSystem& State::GetAudioSystem() { return m_App->GetAudioSystem(); }
UIRenderSystem& State::GetUIRenderSystem() { return m_App->GetUIRenderSystem(); }
UIInteractSystem& State::GetUIInteractSystem() { return m_App->GetUIInteractSystem(); }
ScriptableSystem& State::GetScriptSystem() { return m_App->GetScriptSystem(); }
ParticleSystem& State::GetParticleSystem() { return m_App->GetParticleSystem(); }
SkyboxRenderSystem& State::GetSkyboxRenderSystem() { return m_App->GetSkyboxRenderSystem(); }
AnimationSystem& State::GetAnimationSystem() { return m_App->GetAnimationSystem(); }

SceneManager& State::GetSceneManager() { return m_App->GetSceneManager(); }
ResourceManager& State::GetResourceManager() { return m_App->GetResourceManager(); }
SoundManager& State::GetSoundManager() { return m_App->GetSoundManager(); }
AppHandler& State::GetAppHandler() { return m_App->GetAppHandler(); }
InputManager& State::GetInputManager() { return GetAppHandler().GetInputManager(); }
KeyboardManager& State::GetKeyboard() { return GetAppHandler().GetKeyboard(); }
MouseManager& State::GetMouse() { return GetAppHandler().GetMouse(); }
