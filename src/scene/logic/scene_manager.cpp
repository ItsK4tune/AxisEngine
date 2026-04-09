#include <algorithm>
#include <audio/logic/audio_service.h>
#include <core/logic/config_manager.h>
#include <physics/interface/i_physics_world.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <scene/type/scene_events.h>
#include <resource/logic/resource_manager.h>
#include <resource/type/resource_events.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>

namespace {
    std::string SceneBasename(const std::string &filePath)
    {
        size_t slash = filePath.find_last_of("/\\");
        std::string name = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;
        size_t dot = name.rfind('.');
        if (dot != std::string::npos)
            name = name.substr(0, dot);
        return name;
    }
}

SceneManager::SceneManager() {}

void SceneManager::Initialize()
{

    auto& sl = ServiceLocator::Instance();
    auto& scene = sl.Require<Scene>();
    EventManager::Instance().Publish(SceneChangedEvent{ &scene.registry, &scene });

    LOGGER_INFO("SceneManager") << "Initialized";
}

void SceneManager::Shutdown()
{
    ClearAllIncludingPersistent();
    LOGGER_INFO("SceneManager") << "Shutdown";
}

void SceneManager::AddEntity(entt::entity entity, const std::string &sceneName)
{
    for (auto &rec : m_LoadedScenes)
    {
        if (rec.name == sceneName || rec.filePath == sceneName)
        {
            rec.entities.push_back(entity);
            return;
        }
    }
}

void SceneManager::LoadScene(const std::string& filePath, bool persistent)
{
    LOGGER_INFO("SceneManager") << "Loading scene: " << filePath << (persistent ? " (persistent)" : "");

    if (IsLoaded(filePath))
    {
        LOGGER_WARN("SceneManager") << "Scene already loaded: " << filePath;
        return;
    }

    auto& sl = ServiceLocator::Instance();
    auto& scene = sl.Require<Scene>();
    auto& resources = sl.Require<ResourceManager>();
    auto* physics = sl.Resolve<IPhysicsWorld>();
    auto& audio = sl.Require<AudioService>();

    SceneLoadResult res = SceneSerializer::Deserialize(filePath, scene, resources, physics, audio);

    SceneRecord rec;
    rec.filePath = filePath;
    rec.name = SceneBasename(filePath);
    rec.persistent = persistent;
    rec.loadOrder = m_NextLoadOrder++;
    rec.entities = std::move(res.entities);
    rec.ownedShaders = std::move(res.loadedShaders);
    rec.ownedModels = std::move(res.loadedModels);
    rec.ownedTextures = std::move(res.loadedTextures);
    rec.ownedFonts = std::move(res.loadedFonts);
    rec.ownedSkyboxes = std::move(res.loadedSkyboxes);
    rec.ownedAnimations = std::move(res.loadedAnimations);
    rec.ownedSounds = std::move(res.loadedSounds);
    rec.appliedConfig = res.appliedConfig;
    rec.hasConfig = res.hasConfig;

    if (rec.hasConfig)
    {
        ServiceLocator::Instance().Require<ConfigManager>().UpdateConfig(rec.appliedConfig);
    }

    m_LoadedScenes.push_back(std::move(rec));
    EventManager::Instance().Publish(SceneLoadedEvent{filePath});
}

void SceneManager::UnloadScene(const std::string& filePath)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return r.filePath == filePath || r.name == filePath;
    });

    if (it != m_LoadedScenes.end())
    {
        Internal_UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        Internal_ReindexScenes();
    }
}

void SceneManager::UnloadScene(const SceneRecord* rec)
{
    if (!rec) return;
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return &r == rec;
    });

    if (it != m_LoadedScenes.end())
    {
        Internal_UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        Internal_ReindexScenes();
    }
}

void SceneManager::UnloadSceneByName(const std::string& name)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return r.name == name;
    });

    if (it != m_LoadedScenes.end())
    {
        Internal_UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        Internal_ReindexScenes();
    }
}

void SceneManager::UnloadSceneByOrder(int order)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return r.loadOrder == order;
    });

    if (it != m_LoadedScenes.end())
    {
        Internal_UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        Internal_ReindexScenes();
    }
}

void SceneManager::PopScene()
{
    if (m_LoadedScenes.empty()) return;

    for (int i = (int)m_LoadedScenes.size() - 1; i >= 0; --i)
    {
        if (!m_LoadedScenes[i].persistent)
        {
            Internal_UnloadRecord(m_LoadedScenes[i]);
            m_LoadedScenes.erase(m_LoadedScenes.begin() + i);
            Internal_ReindexScenes();
            return;
        }
    }
}

void SceneManager::ChangeScene(const std::string& filePath)
{
    ClearAllScenes();
    LoadScene(filePath, false);
    

    auto& sl = ServiceLocator::Instance();
    auto& scene = sl.Require<Scene>();
    EventManager::Instance().Publish(SceneChangedEvent{ &scene.registry, &scene });
}

void SceneManager::ClearAllScenes()
{
    for (auto it = m_LoadedScenes.begin(); it != m_LoadedScenes.end(); )
    {
        if (!it->persistent)
        {
            Internal_UnloadRecord(*it);
            it = m_LoadedScenes.erase(it);
        }
        else
        {
            ++it;
        }
    }
    Internal_ReindexScenes();
}

void SceneManager::ClearAllIncludingPersistent()
{
    for (auto& rec : m_LoadedScenes)
    {
        Internal_UnloadRecord(rec);
    }
    m_LoadedScenes.clear();
    m_NextLoadOrder = 0;
}

void SceneManager::QueueLoadScene(const std::string& path, bool persistent)
{
    m_PendingQueue.push_back({PendingOp::Load, path, persistent});
}

void SceneManager::QueueUnloadScene(const std::string& path)
{
    m_PendingQueue.push_back({PendingOp::Unload, path, false});
}

void SceneManager::QueueChangeScene(const std::string& path)
{
    m_PendingQueue.push_back({PendingOp::Change, path, false});
}

void SceneManager::QueuePopScene()
{
    m_PendingQueue.push_back({PendingOp::Pop, "", false});
}

void SceneManager::UpdatePendingScene()
{
    std::vector<PendingOp> ops = std::move(m_PendingQueue);
    m_PendingQueue.clear();

    for (const auto& op : ops)
    {
        switch (op.type)
        {
        case PendingOp::Load:
            LoadScene(op.path, op.persistent);
            break;
        case PendingOp::Pop:
            PopScene();
            break;
        case PendingOp::Change:
            ChangeScene(op.path);
            break;
        case PendingOp::Unload:
            UnloadScene(op.path);
            break;
        }
    }
}

bool SceneManager::IsLoaded(const std::string& filePath) const
{
    return GetScene(filePath) != nullptr;
}

const SceneRecord* SceneManager::GetScene(const std::string& filePath) const
{
    for (const auto& rec : m_LoadedScenes)
    {
        if (rec.filePath == filePath || rec.name == filePath) return &rec;
    }
    return nullptr;
}

const SceneRecord* SceneManager::GetSceneByName(const std::string& name) const
{
    for (const auto& rec : m_LoadedScenes)
    {
        if (rec.name == name) return &rec;
    }
    return nullptr;
}

IPhysicsWorld* SceneManager::GetPhysicsWorld() 
{ 
    return ServiceLocator::Instance().Resolve<IPhysicsWorld>(); 
}

const SceneRecord* SceneManager::GetSceneByOrder(int order) const
{
    for (const auto& rec : m_LoadedScenes)
    {
        if (rec.loadOrder == order) return &rec;
    }
    return nullptr;
}

std::vector<const SceneRecord*> SceneManager::GetScenes() const
{
    std::vector<const SceneRecord*> res;
    for (const auto& rec : m_LoadedScenes) res.push_back(&rec);
    return res;
}

void SceneManager::LogScene(const std::string& filePath) const
{
    const SceneRecord* rec = GetScene(filePath);
    if (rec)
    {
        LOGGER_INFO("SceneManager") << "Scene: " << rec->name << " (" << rec->filePath << "), Entities: " << rec->entities.size();
    }
}

void SceneManager::LogAllScenes() const
{
    LOGGER_INFO("SceneManager") << "Loaded Scenes (" << m_LoadedScenes.size() << "):";
    for (const auto& rec : m_LoadedScenes)
    {
        LOGGER_INFO("SceneManager") << " - " << rec.name << " (Order: " << rec.loadOrder << ", Persistent: " << (rec.persistent ? "Yes" : "No") << ")";
    }
}

void SceneManager::Internal_UnloadRecord(SceneRecord& rec)
{
    LOGGER_INFO("SceneManager") << "Unloading scene: " << rec.filePath;
    Internal_DestroySceneEntities(rec);
    Internal_UnloadOrphanedResources(rec);
    EventManager::Instance().Publish(SceneUnloadedEvent{rec.filePath});
}

void SceneManager::Internal_DestroySceneEntities(SceneRecord& rec)
{
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    for (auto entity : rec.entities)
    {
        if (scene.registry.valid(entity))
        {
            scene.registry.destroy(entity);
        }
    }
    rec.entities.clear();
}

void SceneManager::Internal_UnloadOrphanedResources(const SceneRecord& rec)
{
    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    
    for (const auto& shader : rec.ownedShaders) resources.UnloadShader(shader);
    for (const auto& model : rec.ownedModels) resources.UnloadModel(model);
    for (const auto& texture : rec.ownedTextures) resources.UnloadTexture(texture);
    for (const auto& font : rec.ownedFonts) resources.UnloadFont(font);
    for (const auto& sky : rec.ownedSkyboxes) resources.UnloadSkybox(sky);
    for (const auto& anim : rec.ownedAnimations) resources.UnloadAnimation(anim);
    for (const auto& sound : rec.ownedSounds) resources.UnloadSound(sound);
}

void SceneManager::Internal_RollbackConfig(const SceneRecord& removed)
{


    if (removed.hasConfig)
    {
        LOGGER_INFO("SceneManager") << "Scene with config removed: " << removed.name;
    }
}

void SceneManager::Internal_ReindexScenes()
{
    m_NextLoadOrder = 0;
    for (auto& rec : m_LoadedScenes)
    {
        rec.loadOrder = m_NextLoadOrder++;
    }
}
