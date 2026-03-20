#include <core/app/runtime_core.h>
#include <core/type/app_config.h>
#include <core/logic/config_loader.h>
#include <engine/platform/logic/io_handler.h>
#include <core/app/system_manager.h>
#include <platform/logic/input_loader.h>
#include <audio/logic/audio_service.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/scriptable_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/ui_render_system.h>
#include <ecs/logic/video_system.h>
#include <navigation/logic/navigation_system.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>

RenderSystem&       EngineAccessor::GetRenderSystem() const       { return GetSystem<RenderSystem>(); }
PhysicsSystem&      EngineAccessor::GetPhysicsSystem() const      { return GetSystem<PhysicsSystem>(); }
AudioSystem&        EngineAccessor::GetAudioSystem() const        { return GetSystem<AudioSystem>(); }
UIRenderSystem&     EngineAccessor::GetUIRenderSystem() const     { return GetSystem<UIRenderSystem>(); }
ScriptableSystem&   EngineAccessor::GetScriptSystem() const       { return GetSystem<ScriptableSystem>(); }
ParticleSystem&     EngineAccessor::GetParticleSystem() const     { return GetSystem<ParticleSystem>(); }
SkyboxRenderSystem& EngineAccessor::GetSkyboxRenderSystem() const { return GetSystem<SkyboxRenderSystem>(); }
AnimationSystem&    EngineAccessor::GetAnimationSystem() const    { return GetSystem<AnimationSystem>(); }
VideoSystem&        EngineAccessor::GetVideoSystem() const        { return GetSystem<VideoSystem>(); }
NavigationSystem&   EngineAccessor::GetNavigationSystem() const   { return GetSystem<NavigationSystem>(); }


Scene&           EngineAccessor::GetScene() const           { return *m_ActiveScene; }
SceneManager&    EngineAccessor::GetSceneManager() const    { return Get<SceneManager>(); }
ResourceManager& EngineAccessor::GetResourceManager() const { return Get<ResourceManager>(); }
AudioService&    EngineAccessor::GetAudioService() const     { return Get<AudioService>(); }
IOHandler&       EngineAccessor::GetIOHandler() const       { return Get<IOHandler>(); }
InputManager&    EngineAccessor::GetInputManager() const    { return Get<IOHandler>().GetInputManager(); }
KeyboardManager& EngineAccessor::GetKeyboard() const        { return Get<IOHandler>().GetKeyboard(); }
MouseManager&    EngineAccessor::GetMouse() const           { return Get<IOHandler>().GetMouse(); }
RuntimeCore&     EngineAccessor::GetRuntimeCore() const     { return Get<RuntimeCore>(); }
SystemManager&   EngineAccessor::GetSystemManager() const  { return Get<SystemManager>(); }


void EngineAccessor::LoadScene(const std::string& path, bool persistent) { ServiceLocator::Instance().Require<SceneManager>().LoadScene(path, persistent); }
void EngineAccessor::LoadInputBindings(const std::string& path) { InputLoader::LoadBindings(path, GetInputManager()); }
void EngineAccessor::QueueLoadScene(const std::string& path, bool persistent) { ServiceLocator::Instance().Require<SceneManager>().QueueLoadScene(path, persistent); }
void EngineAccessor::UnloadScene(const std::string& path)  { ServiceLocator::Instance().Require<SceneManager>().UnloadScene(path); }
void EngineAccessor::UnloadScene(const SceneRecord* rec)   { ServiceLocator::Instance().Require<SceneManager>().UnloadScene(rec); }
void EngineAccessor::ChangeScene(const std::string& path)  { ServiceLocator::Instance().Require<SceneManager>().ChangeScene(path); }
void EngineAccessor::PopScene()                            { ServiceLocator::Instance().Require<SceneManager>().PopScene(); }
void EngineAccessor::QueuePopScene()                       { ServiceLocator::Instance().Require<SceneManager>().QueuePopScene(); }
std::vector<const SceneRecord*> EngineAccessor::GetScenes(){ return ServiceLocator::Instance().Require<SceneManager>().GetScenes(); }
void EngineAccessor::SetCursorMode(CursorMode mode) { ServiceLocator::Instance().Require<IOHandler>().GetMouse().SetCursorMode(mode); }

void EngineAccessor::EnablePhysics(bool e)    { GetPhysicsSystem().SetEnabled(e); }
void EngineAccessor::EnableRender(bool e)    { GetRenderSystem().SetEnabled(e); }
void EngineAccessor::EnableAudio(bool e)    { GetAudioSystem().SetEnabled(e); }
void EngineAccessor::EnableScript(bool e)    { GetScriptSystem().SetEnabled(e); }
void EngineAccessor::EnableAnimation(bool e)    { GetAnimationSystem().SetEnabled(e); }
void EngineAccessor::EnableVideo(bool e)    { GetVideoSystem().SetEnabled(e); }
void EngineAccessor::EnableUIRender(bool e)    { GetUIRenderSystem().SetEnabled(e); }
void EngineAccessor::EnableParticle(bool e)    { GetParticleSystem().SetEnabled(e); }
void EngineAccessor::EnableSkybox(bool e)    { GetSkyboxRenderSystem().SetEnabled(e); }
void EngineAccessor::EnableNavigation(bool e) { GetNavigationSystem().SetEnabled(e); }

void EngineAccessor::EnableLogic(bool enable)
{
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
    EnableNavigation(enable);
}

bool EngineAccessor::GetAction(const std::string &name) const { return ServiceLocator::Instance().Require<IOHandler>().GetInputManager().GetAction(name); }
bool EngineAccessor::GetActionDown(const std::string &name) const { return ServiceLocator::Instance().Require<IOHandler>().GetInputManager().GetActionDown(name); }
bool EngineAccessor::GetActionUp(const std::string &name) const { return ServiceLocator::Instance().Require<IOHandler>().GetInputManager().GetActionUp(name); }

void EngineAccessor::SetTimeScale(float scale) { ServiceLocator::Instance().Require<RuntimeCore>().SetTimeScale(scale); }
float EngineAccessor::GetTimeScale() const { return ServiceLocator::Instance().Require<RuntimeCore>().GetTimeScale(); }
float EngineAccessor::GetRealDeltaTime() const { return ServiceLocator::Instance().Require<RuntimeCore>().GetRealDeltaTime(); }

const AppConfig& EngineAccessor::GetConfig() const { return ServiceLocator::Instance().Require<ConfigManager>().GetConfig(); }
void EngineAccessor::ApplyConfig(const AppConfig& config) { 
    ServiceLocator::Instance().Require<ConfigManager>().UpdateConfig(config);
    EventSystem::Instance().Publish(ConfigChangedEvent{config});
}








