#include <core/app/engine_accessor.h>
#include <audio/logic/audio_service.h>
#include <core/app/runtime_core.h>
#include <core/logic/config_loader.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/data_manager.h>
#include <core/logic/data_node_serializer.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>

Scene& EngineAccessor::GetScene() const
{
    return *m_ActiveScene;
}

void EngineAccessor::LoadScene(const std::string& path, bool persistent)
{
    ServiceLocator::Instance().Require<SceneManager>().LoadScene(path, persistent);
}
bool EngineAccessor::LoadInputBindings(const std::string& path)
{
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        InputSerializer serializer;
        return serializer.Deserialize(path, io->GetInputManager());
    }
    return false;
}
bool EngineAccessor::SaveInputBindings(const std::string& path)
{
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        InputSerializer serializer;
        return serializer.Serialize(path, io->GetInputManager());
    }
    return false;
}
bool EngineAccessor::LoadDataNodes(const std::string& path)
{
    if (auto* dm = ServiceLocator::Instance().Resolve<DataManager>())
    {
        DataNodeSerializer serializer;
        return serializer.Deserialize(path, dm->GetDataNodes());
    }
    return false;
}
bool EngineAccessor::SaveDataNodes(const std::string& path)
{
    if (auto* dm = ServiceLocator::Instance().Resolve<DataManager>())
    {
        DataNodeSerializer serializer;
        return serializer.Serialize(path, dm->GetDataNodes());
    }
    return false;
}

void EngineAccessor::SetCursorMode(CursorMode mode)
{
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
        io->GetMouse().SetCursorMode(mode);
}

void EngineAccessor::LoadLanguage(const std::string& path, const std::string& name)
{
    if (auto* loc = Resolve<LocalizationSystem>())
        loc->LoadLanguage(path, name);
}

void EngineAccessor::SetLanguage(const std::string& name)
{
    if (auto* loc = Resolve<LocalizationSystem>())
        loc->SetLanguage(name);
}

std::string EngineAccessor::GetLanguage() const
{
    if (auto* loc = Resolve<LocalizationSystem>())
        return loc->GetLanguage();
    return "";
}

std::string EngineAccessor::GetTranslation(const std::string& key) const
{
    if (auto* loc = Resolve<LocalizationSystem>())
        return loc->Get(key);
    return "[MISSING: " + key + "]";
}

void EngineAccessor::QueueLoadScene(const std::string& path, bool persistent)
{
    ServiceLocator::Instance().Require<SceneManager>().QueueLoadScene(path, persistent);
}
void EngineAccessor::UnloadScene(const std::string& path)
{
    ServiceLocator::Instance().Require<SceneManager>().UnloadScene(path);
}
void EngineAccessor::UnloadScene(const SceneRecord* rec)
{
    ServiceLocator::Instance().Require<SceneManager>().UnloadScene(rec);
}
void EngineAccessor::ChangeScene(const std::string& path)
{
    ServiceLocator::Instance().Require<SceneManager>().ChangeScene(path);
}
void EngineAccessor::PopScene()
{
    ServiceLocator::Instance().Require<SceneManager>().PopScene();
}
void EngineAccessor::QueuePopScene()
{
    ServiceLocator::Instance().Require<SceneManager>().QueuePopScene();
}
std::vector<const SceneRecord*> EngineAccessor::GetScenes()
{
    return ServiceLocator::Instance().Require<SceneManager>().GetScenes();
}
bool EngineAccessor::IsSceneLoaded(const std::string& path)
{
    return ServiceLocator::Instance().Require<SceneManager>().IsLoaded(path);
}

void EngineAccessor::EnableSystem(const std::string& systemName, bool enable)
{
    EventManager::Instance().Publish(SystemEnabledEvent{systemName, enable});
}

void EngineAccessor::EnableLogic(bool enable)
{
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
    EnableNavigation(enable);
}

bool EngineAccessor::GetAction(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetAction(name) : false;
}
bool EngineAccessor::GetActionDown(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetActionDown(name) : false;
}
bool EngineAccessor::GetActionUp(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetActionUp(name) : false;
}

void EngineAccessor::SetTimeScale(float scale)
{
    ServiceLocator::Instance().Require<RuntimeCore>().GetEngineLoop().SetTimeScale(scale);
}
float EngineAccessor::GetTimeScale() const
{
    return ServiceLocator::Instance().Require<RuntimeCore>().GetEngineLoop().GetTimeScale();
}
float EngineAccessor::GetRealDeltaTime() const
{
    return ServiceLocator::Instance().Require<RuntimeCore>().GetEngineLoop().GetRealDeltaTime();
}

const AppConfig& EngineAccessor::GetConfig() const
{
    return ServiceLocator::Instance().Require<ConfigManager>().GetConfig();
}
void EngineAccessor::ApplyConfig(const AppConfig& config)
{
    ServiceLocator::Instance().Require<ConfigManager>().UpdateConfig(config);
    EventManager::Instance().Publish(ConfigChangedEvent{config});
}
