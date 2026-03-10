#include <algorithm>
#include <core/logic/app_framework.h>
#include <ecs/manager/entity_manager.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/scene_serializer.h>
#include <core/logic/logger.h>

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

void SceneManager::Initialize(EngineContext ctx, std::function<void(const AppConfig&)> applyConfigFn)
{
    m_Ctx = ctx;
    m_Scene = ctx.scene;
    m_Resources = ctx.resources;
    m_Physics = ctx.physics;
    m_SoundPlayer = ctx.soundPlayer;
    m_ApplyConfigFn = std::move(applyConfigFn);
    
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

    SceneLoadResult res = SceneSerializer::Deserialize(filePath, *m_Scene, *m_Resources, *m_Physics, *m_SoundPlayer, m_Ctx);

    SceneRecord rec;
    rec.filePath = filePath;
    rec.name = SceneBasename(filePath);
    rec.persistent = persistent;
    rec.loadOrder = m_nextLoadOrder++;
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

    if (rec.hasConfig && m_ApplyConfigFn)
    {
        m_ApplyConfigFn(rec.appliedConfig);
    }

    m_LoadedScenes.push_back(std::move(rec));
}

void SceneManager::UnloadScene(const std::string& filePath)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return r.filePath == filePath || r.name == filePath;
    });

    if (it != m_LoadedScenes.end())
    {
        _UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        _ReindexScenes();
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
        _UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        _ReindexScenes();
    }
}

void SceneManager::UnloadSceneByName(const std::string& name)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return r.name == name;
    });

    if (it != m_LoadedScenes.end())
    {
        _UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        _ReindexScenes();
    }
}

void SceneManager::UnloadSceneByOrder(int order)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(), [&](const SceneRecord& r) {
        return r.loadOrder == order;
    });

    if (it != m_LoadedScenes.end())
    {
        _UnloadRecord(*it);
        m_LoadedScenes.erase(it);
        _ReindexScenes();
    }
}

void SceneManager::PopScene()
{
    if (m_LoadedScenes.empty()) return;

    for (int i = (int)m_LoadedScenes.size() - 1; i >= 0; --i)
    {
        if (!m_LoadedScenes[i].persistent)
        {
            _UnloadRecord(m_LoadedScenes[i]);
            m_LoadedScenes.erase(m_LoadedScenes.begin() + i);
            _ReindexScenes();
            return;
        }
    }
}

void SceneManager::ChangeScene(const std::string& filePath)
{
    ClearAllScenes();
    LoadScene(filePath, false);
}

void SceneManager::ClearAllScenes()
{
    for (auto it = m_LoadedScenes.begin(); it != m_LoadedScenes.end(); )
    {
        if (!it->persistent)
        {
            _UnloadRecord(*it);
            it = m_LoadedScenes.erase(it);
        }
        else
        {
            ++it;
        }
    }
    _ReindexScenes();
}

void SceneManager::ClearAllIncludingPersistent()
{
    for (auto& rec : m_LoadedScenes)
    {
        _UnloadRecord(rec);
    }
    m_LoadedScenes.clear();
    m_nextLoadOrder = 0;
}

void SceneManager::QueueLoadScene(const std::string& path, bool persistent)
{
    m_pendingQueue.push_back({PendingOp::Load, path, persistent});
}

void SceneManager::QueueUnloadScene(const std::string& path)
{
    m_pendingQueue.push_back({PendingOp::Unload, path, false});
}

void SceneManager::QueueChangeScene(const std::string& path)
{
    m_pendingQueue.push_back({PendingOp::Change, path, false});
}

void SceneManager::QueuePopScene()
{
    m_pendingQueue.push_back({PendingOp::Pop, "", false});
}

void SceneManager::UpdatePendingScene()
{
    std::vector<PendingOp> ops = std::move(m_pendingQueue);
    m_pendingQueue.clear();

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

void SceneManager::_UnloadRecord(SceneRecord& rec)
{
    LOGGER_INFO("SceneManager") << "Unloading scene: " << rec.filePath;
    _DestroySceneEntities(rec);
    _UnloadOrphanedResources(rec);
}

void SceneManager::_DestroySceneEntities(SceneRecord& rec)
{
    for (auto entity : rec.entities)
    {
        if (m_Scene->registry.valid(entity))
        {
            m_Scene->registry.destroy(entity);
        }
    }
    rec.entities.clear();
}

void SceneManager::_UnloadOrphanedResources(const SceneRecord& rec)
{
    // Implementation for resource unloading if needed
}

void SceneManager::_RollbackConfig(const SceneRecord& removed)
{
    // If the removed scene had a config, we might want to restore a previous one
    // for simplicity, we just log it for now
    if (removed.hasConfig)
    {
        LOGGER_INFO("SceneManager") << "Scene with config removed: " << removed.name;
    }
}

void SceneManager::_ReindexScenes()
{
    m_nextLoadOrder = 0;
    for (auto& rec : m_LoadedScenes)
    {
        rec.loadOrder = m_nextLoadOrder++;
    }
}
