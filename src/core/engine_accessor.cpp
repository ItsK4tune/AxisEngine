#include <core/engine_accessor.h>
#include <core/app/config_loader.h>
#include <core/engine_context.h>
#include <systems/window/io_handler.h>
#include <core/runtime_core.h>
#include <core/system_manager.h>
#include <systems/audio/sound_player.h>
#include <ecs/systems/animation_system.h>
#include <ecs/systems/audio_system.h>
#include <ecs/systems/particle_system.h>
#include <ecs/systems/physics_system.h>
#include <ecs/systems/render_system.h>
#include <ecs/systems/script_system.h>
#include <ecs/systems/skybox_system.h>
#include <ecs/systems/ui_system.h>
#include <ecs/systems/video_system.h>
#include <systems/input/input_loader.h>
#include <systems/input/input_manager.h>
#include <systems/input/keyboard_manager.h>
#include <systems/input/mouse_manager.h>
#include <resource/resource_manager.h>
#include <scene/scene.h>
#include <scene/scene_manager.h>

RenderSystem&       EngineAccessor::GetRenderSystem() const       { return *m_Ctx.systems->GetSystem<RenderSystem>(); }
PhysicsSystem&      EngineAccessor::GetPhysicsSystem() const      { return *m_Ctx.systems->GetSystem<PhysicsSystem>(); }
AudioSystem&        EngineAccessor::GetAudioSystem() const        { return *m_Ctx.systems->GetSystem<AudioSystem>(); }
UIRenderSystem&     EngineAccessor::GetUIRenderSystem() const     { return *m_Ctx.systems->GetSystem<UIRenderSystem>(); }
ScriptableSystem&   EngineAccessor::GetScriptSystem() const       { return *m_Ctx.systems->GetSystem<ScriptableSystem>(); }
ParticleSystem&     EngineAccessor::GetParticleSystem() const     { return *m_Ctx.systems->GetSystem<ParticleSystem>(); }
SkyboxRenderSystem& EngineAccessor::GetSkyboxRenderSystem() const { return *m_Ctx.systems->GetSystem<SkyboxRenderSystem>(); }
AnimationSystem&    EngineAccessor::GetAnimationSystem() const    { return *m_Ctx.systems->GetSystem<AnimationSystem>(); }
VideoSystem&        EngineAccessor::GetVideoSystem() const        { return *m_Ctx.systems->GetSystem<VideoSystem>(); }

Scene&           EngineAccessor::GetScene() const           { return *m_Ctx.scene; }
SceneManager&    EngineAccessor::GetSceneManager() const    { return *m_Ctx.sceneManager; }
ResourceManager& EngineAccessor::GetResourceManager() const { return *m_Ctx.resources; }
SoundPlayer&     EngineAccessor::GetSoundPlayer() const     { return *m_Ctx.soundPlayer; }
IOHandler&       EngineAccessor::GetIOHandler() const       { return *m_Ctx.io; }
InputManager&    EngineAccessor::GetInputManager() const    { return m_Ctx.io->GetInputManager(); }
KeyboardManager& EngineAccessor::GetKeyboard() const        { return m_Ctx.io->GetKeyboard(); }
MouseManager&    EngineAccessor::GetMouse() const           { return m_Ctx.io->GetMouse(); }

void EngineAccessor::LoadScene(const std::string& path, bool persistent) { m_Ctx.sceneManager->LoadScene(path, persistent); }
void EngineAccessor::LoadInputBindings(const std::string& path) { InputLoader::LoadBindings(path, GetInputManager()); }
void EngineAccessor::QueueLoadScene(const std::string& path, bool persistent) { m_Ctx.sceneManager->QueueLoadScene(path, persistent); }
void EngineAccessor::UnloadScene(const std::string& path)  { m_Ctx.sceneManager->UnloadScene(path); }
void EngineAccessor::UnloadScene(const SceneRecord* rec)   { m_Ctx.sceneManager->UnloadScene(rec); }
void EngineAccessor::ChangeScene(const std::string& path)  { m_Ctx.sceneManager->ChangeScene(path); }
void EngineAccessor::PopScene()                            { m_Ctx.sceneManager->PopScene(); }
void EngineAccessor::QueuePopScene()                       { m_Ctx.sceneManager->QueuePopScene(); }
std::vector<const SceneRecord*> EngineAccessor::GetScenes(){ return m_Ctx.sceneManager->GetScenes(); }
void EngineAccessor::SetCursorMode(Input::CursorMode mode) { m_Ctx.io->GetMouse().SetCursorMode(mode); }

void EngineAccessor::EnablePhysics(bool e)    { GetPhysicsSystem().SetEnabled(e); }
void EngineAccessor::EnableRender(bool e)    { GetRenderSystem().SetEnabled(e); }
void EngineAccessor::EnableAudio(bool e)    { GetAudioSystem().SetEnabled(e); }
void EngineAccessor::EnableScript(bool e)    { GetScriptSystem().SetEnabled(e); }
void EngineAccessor::EnableAnimation(bool e)    { GetAnimationSystem().SetEnabled(e); }
void EngineAccessor::EnableVideo(bool e)    { GetVideoSystem().SetEnabled(e); }
void EngineAccessor::EnableUIRender(bool e)    { GetUIRenderSystem().SetEnabled(e); }
void EngineAccessor::EnableParticle(bool e)    { GetParticleSystem().SetEnabled(e); }
void EngineAccessor::EnableSkybox(bool e)    { GetSkyboxRenderSystem().SetEnabled(e); }

void EngineAccessor::EnableLogic(bool enable)
{
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
}

bool EngineAccessor::GetAction(const std::string &name) const { return m_Ctx.io->GetInputManager().GetAction(name); }
bool EngineAccessor::GetActionDown(const std::string &name) const { return m_Ctx.io->GetInputManager().GetActionDown(name); }
bool EngineAccessor::GetActionUp(const std::string &name) const { return m_Ctx.io->GetInputManager().GetActionUp(name); }

void EngineAccessor::SetTimeScale(float scale) { m_Ctx.runtime->SetTimeScale(scale); }
float EngineAccessor::GetTimeScale() const { return m_Ctx.runtime->GetTimeScale(); }
float EngineAccessor::GetRealDeltaTime() const { return m_Ctx.runtime->GetRealDeltaTime(); }

const AppConfig& EngineAccessor::GetConfig() const { return m_Ctx.runtime->GetConfig(); }
void EngineAccessor::ApplyConfig(const AppConfig& config) { m_Ctx.runtime->ApplyConfig(config); }
