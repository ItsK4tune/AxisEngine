#include <core/app/engine_accessor.h>
#include <core/app/runtime_core.h>
#include <core/type/app_config.h>
#include <core/logic/config_loader.h>
#include <engine/platform/logic/io_handler.h>
#include <ecs/logic/system_manager.h>
#include <platform/logic/input_loader.h>
#include <audio/logic/audio_service.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/logic/config_manager.h>

// NOTE: Concrete ECS system headers are intentionally NOT included here.
// Get*System() methods resolve via SystemManager::GetSystem<T>() which uses
// a type_index cache populated at registration time. This eliminates the
// Layer 2 (Core) → Layer 4 (ECS Systems) reverse dependency.

// --- Forward-declared concrete system getters ---
// These still return concrete types for backward compatibility with game code,
// but the include dependency has been moved to the call site (game code).
// Engine code should use Get<InterfaceType>() instead.

// Include concrete system headers here so GetSystem<T>() template can resolve.
// This is still in .cpp (not .h), so the header dependency is not leaked.





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

void EngineAccessor::EnableSystem(const std::string& systemName, bool enable) {
    EventSystem::Instance().Publish(SystemEnabledEvent{systemName, enable});
}

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








