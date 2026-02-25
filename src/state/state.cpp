#include <state/state.h>
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

void State::EnablePhysics(bool e)   { m_App->GetPhysicsSystem().SetEnabled(e); }
void State::EnableRender(bool e)    { m_App->GetRenderSystem().SetEnabled(e); }
void State::EnableAudio(bool e)     { m_App->GetAudioSystem().SetEnabled(e); }
void State::EnableScript(bool e)    { m_App->GetScriptSystem().SetEnabled(e); }
void State::EnableAnimation(bool e) { m_App->GetAnimationSystem().SetEnabled(e); }
void State::EnableVideo(bool e)     { m_App->GetVideoSystem().SetEnabled(e); }
void State::EnableUIRender(bool e)  { m_App->GetUIRenderSystem().SetEnabled(e); }
void State::EnableParticle(bool e)  { m_App->GetParticleSystem().SetEnabled(e); }
void State::EnableSkybox(bool e)    { m_App->GetSkyboxRenderSystem().SetEnabled(e); }

const AppConfig& State::GetConfig() const { return m_App->GetConfig(); }
void State::ApplyConfig(const AppConfig& config) { m_App->ApplyConfig(config); }

void State::EnableLogic(bool enable)
{
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
}
