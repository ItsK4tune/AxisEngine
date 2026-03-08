#include <core/logic/engine_core.h>
#include <core/type/app_config.h>
#include <core/logic/config_loader.h>
#include <core/unit/engine_context.h>
#include <engine/platform/logic/io_handler.h>
#include <core/manager/system_manager.h>
#include <audio/logic/sound_player.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/script_system.h>
#include <ecs/logic/skybox_system.h>
#include <ecs/logic/ui_system.h>
#include <ecs/logic/video_system.h>
#include <platform/logic/input_system.h>
#include <resource/manager/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>

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
void EngineAccessor::SetCursorMode(CursorMode mode) { m_Ctx.io->GetMouse().SetCursorMode(mode); }

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








