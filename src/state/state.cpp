#include <state/state.h>
#include <app/system_manager.h>
#include <app/application.h>
#include <app/io_handler.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <input/input_manager.h>
#include <scene/scene.h>
#include <scene/scene_manager.h>
#include <resource/resource_manager.h>
#include <audio/sound_player.h>
#include <ecs/systems/render_system.h>
#include <ecs/systems/physics_system.h>
#include <ecs/systems/audio_system.h>
#include <ecs/systems/ui_system.h>
#include <ecs/systems/script_system.h>
#include <ecs/systems/particle_system.h>
#include <ecs/systems/skybox_system.h>
#include <ecs/systems/animation_system.h>
#include <ecs/systems/video_system.h>

RenderSystem&       State::GetRenderSystem()       { return m_App->GetRenderSystem(); }
PhysicsSystem&      State::GetPhysicsSystem()      { return m_App->GetPhysicsSystem(); }
AudioSystem&        State::GetAudioSystem()        { return m_App->GetAudioSystem(); }
UIRenderSystem&     State::GetUIRenderSystem()     { return m_App->GetUIRenderSystem(); }
ScriptableSystem&   State::GetScriptSystem()       { return m_App->GetScriptSystem(); }
ParticleSystem&     State::GetParticleSystem()     { return m_App->GetParticleSystem(); }
SkyboxRenderSystem& State::GetSkyboxRenderSystem() { return m_App->GetSkyboxRenderSystem(); }
AnimationSystem&    State::GetAnimationSystem()    { return m_App->GetAnimationSystem(); }
VideoSystem&        State::GetVideoSystem()        { return m_App->GetVideoSystem(); }

Scene&           State::GetScene()           { return m_App->GetScene(); }
SceneManager&    State::GetSceneManager()    { return m_App->GetSceneManager(); }
ResourceManager& State::GetResourceManager() { return m_App->GetResourceManager(); }
SoundPlayer&     State::GetSoundPlayer()     { return m_App->GetSoundPlayer(); }
IOHandler&       State::GetIOHandler()       { return m_App->GetIOHandler(); }
InputManager&    State::GetInputManager()    { return m_App->GetInputManager(); }
KeyboardManager& State::GetKeyboard()        { return m_App->GetKeyboard(); }
MouseManager&    State::GetMouse()           { return m_App->GetMouse(); }

void State::LoadScene(const std::string& path, bool persistent) { m_App->GetSceneManager().LoadScene(path, persistent); }
void State::QueueLoadScene(const std::string& path, bool persistent) { m_App->GetSceneManager().QueueLoadScene(path, persistent); }
void State::UnloadScene(const std::string& path)  { m_App->GetSceneManager().UnloadScene(path); }
void State::UnloadScene(const SceneRecord* rec)   { m_App->GetSceneManager().UnloadScene(rec); }
void State::ChangeScene(const std::string& path)  { m_App->GetSceneManager().ChangeScene(path); }
void State::PopScene()                            { m_App->GetSceneManager().PopScene(); }
void State::QueuePopScene()                       { m_App->GetSceneManager().QueuePopScene(); }
bool State::IsSceneLoaded(const std::string& path){ return m_App->GetSceneManager().IsLoaded(path); }
void State::LogAllScenes()                        { m_App->GetSceneManager().LogAllScenes(); }
std::vector<const SceneRecord*> State::GetScenes(){ return m_App->GetSceneManager().GetScenes(); }
void State::SetCursorMode(Input::CursorMode mode) { m_App->GetMouse().SetCursorMode(mode); }

void State::EnablePhysics(bool e)   { if (e) m_SystemMask |= (uint32_t)SystemGroup::Physics; else m_SystemMask &= ~(uint32_t)SystemGroup::Physics; }
void State::EnableRender(bool e)    { if (e) m_SystemMask |= (uint32_t)SystemGroup::Render; else m_SystemMask &= ~(uint32_t)SystemGroup::Render; }
void State::EnableAudio(bool e)     { if (e) m_SystemMask |= (uint32_t)SystemGroup::Audio; else m_SystemMask &= ~(uint32_t)SystemGroup::Audio; }
void State::EnableScript(bool e)    { if (e) m_SystemMask |= (uint32_t)SystemGroup::Script; else m_SystemMask &= ~(uint32_t)SystemGroup::Script; }
void State::EnableAnimation(bool e) { if (e) m_SystemMask |= (uint32_t)SystemGroup::Animation; else m_SystemMask &= ~(uint32_t)SystemGroup::Animation; }
void State::EnableVideo(bool e)     { if (e) m_SystemMask |= (uint32_t)SystemGroup::Video; else m_SystemMask &= ~(uint32_t)SystemGroup::Video; }
void State::EnableUIRender(bool e)  { if (e) m_SystemMask |= (uint32_t)SystemGroup::UI; else m_SystemMask &= ~(uint32_t)SystemGroup::UI; }
void State::EnableParticle(bool e)  { if (e) m_SystemMask |= (uint32_t)SystemGroup::Particle; else m_SystemMask &= ~(uint32_t)SystemGroup::Particle; }
void State::EnableSkybox(bool e)    { if (e) m_SystemMask |= (uint32_t)SystemGroup::Skybox; else m_SystemMask &= ~(uint32_t)SystemGroup::Skybox; }

const AppConfig& State::GetConfig() const { return m_App->GetConfig(); }
void State::ApplyConfig(const AppConfig& config) { m_App->ApplyConfig(config); }

void State::EnableLogic(bool enable)
{
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
}
