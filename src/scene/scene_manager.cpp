#include <scene/scene_manager.h>
#include <utils/logger.h>
#include <scene/scene_serializer.h>
#include <app/application.h>
#include <algorithm>
#include <ecs/entity_manager.h>

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

SceneManager::SceneManager(Scene &scene, ResourceManager &res, IPhysicsWorld &phys, SoundPlayer &sound, Application *app)
    : m_Scene(scene), m_Resources(res), m_Physics(phys), m_SoundPlayer(sound), m_App(app) {}

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

void SceneManager::LoadScene(const std::string &filePath, bool persistent)
{
    if (IsLoaded(filePath))
    {
        LOGGER_INFO("SceneManager") << "Scene already loaded: " << filePath;
        return;
    }

    bool isLoadAxs = (SceneBasename(filePath) == "load");

    SceneLoadResult result = SceneSerializer::Deserialize(filePath, m_Scene, m_Resources, m_Physics, m_SoundPlayer, m_App);

    if (result.entities.empty() && !result.hasConfig)
    {
        LOGGER_WARN("SceneManager") << "Scene loaded but has no entities or config: " << filePath;
    }

    SceneRecord rec;
    rec.name = SceneBasename(filePath);
    rec.filePath = filePath;
    rec.loadOrder = m_nextLoadOrder++;
    rec.persistent = persistent || isLoadAxs;
    rec.inviolable = isLoadAxs;
    rec.entities = std::move(result.entities);
    rec.ownedShaders = std::move(result.loadedShaders);
    rec.ownedModels = std::move(result.loadedModels);
    rec.ownedTextures = std::move(result.loadedTextures);
    rec.ownedFonts = std::move(result.loadedFonts);
    rec.ownedSkyboxes = std::move(result.loadedSkyboxes);
    rec.ownedAnimations = std::move(result.loadedAnimations);
    rec.ownedSounds = std::move(result.loadedSounds);
    rec.hasConfig = result.hasConfig;
    rec.appliedConfig = result.appliedConfig;

    LOGGER_INFO("SceneManager") << "Scene loaded: " << filePath
                                << " [order=" << rec.loadOrder << ", persistent=" << rec.persistent
                                << ", inviolable=" << rec.inviolable
                                << ", entities=" << rec.entities.size() << "]";

    m_LoadedScenes.push_back(std::move(rec));
}

void SceneManager::_DestroySceneEntities(SceneRecord &rec)
{
    for (auto entity : rec.entities)
    {
        if (m_Scene.registry.valid(entity))
            EntityManager::DestroyEntity(m_Scene, entity, this);
    }
    rec.entities.clear();
}

void SceneManager::_UnloadOrphanedResources(const SceneRecord &rec)
{
    auto isShared = [&](const std::string &name, const std::vector<std::string> &list) -> bool
    {
        for (const auto &n : list)
            if (n == name)
                return true;
        return false;
    };

    auto usedByOther = [&](const std::string &name,
                           std::vector<std::string> SceneRecord::*field) -> bool
    {
        for (const auto &other : m_LoadedScenes)
        {
            if (other.filePath == rec.filePath)
                continue;
            if (isShared(name, other.*field))
                return true;
        }
        return false;
    };

    for (const auto &name : rec.ownedShaders)
        if (!usedByOther(name, &SceneRecord::ownedShaders))
            m_Resources.UnloadShader(name);
    for (const auto &name : rec.ownedModels)
        if (!usedByOther(name, &SceneRecord::ownedModels))
            m_Resources.UnloadModel(name);
    for (const auto &name : rec.ownedTextures)
        if (!usedByOther(name, &SceneRecord::ownedTextures))
            m_Resources.UnloadTexture(name);
    for (const auto &name : rec.ownedFonts)
        if (!usedByOther(name, &SceneRecord::ownedFonts))
            m_Resources.UnloadFont(name);
    for (const auto &name : rec.ownedSkyboxes)
        if (!usedByOther(name, &SceneRecord::ownedSkyboxes))
            m_Resources.UnloadSkybox(name);
    for (const auto &name : rec.ownedAnimations)
        if (!usedByOther(name, &SceneRecord::ownedAnimations))
            m_Resources.UnloadAnimation(name);
    for (const auto &name : rec.ownedSounds)
        if (!usedByOther(name, &SceneRecord::ownedSounds))
            m_Resources.UnloadSound(name);
}

void SceneManager::_RollbackConfig(const SceneRecord &removed)
{
    if (!removed.hasConfig)
        return;

    const SceneRecord *best = nullptr;
    for (const auto &rec : m_LoadedScenes)
    {
        if (rec.filePath == removed.filePath)
            continue;
        if (!rec.hasConfig)
            continue;
        if (!best || rec.loadOrder > best->loadOrder)
            best = &rec;
    }

    if (best && m_App)
    {
        LOGGER_INFO("SceneManager") << "Rolling back config to scene: " << best->name;
        m_App->ApplyConfig(best->appliedConfig);
    }
}

void SceneManager::_UnloadRecord(SceneRecord &rec)
{
    _DestroySceneEntities(rec);
    _UnloadOrphanedResources(rec);
    _RollbackConfig(rec);
}

void SceneManager::UnloadScene(const std::string &filePath)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(),
                           [&](const SceneRecord &r)
                           { return r.filePath == filePath; });

    if (it == m_LoadedScenes.end())
    {
        LOGGER_WARN("SceneManager") << "Scene not found or not loaded: " << filePath;
        return;
    }
    if (it->inviolable)
    {
        LOGGER_WARN("SceneManager") << "Cannot unload inviolable scene: " << filePath;
        return;
    }

    LOGGER_INFO("SceneManager") << "Unloading scene: " << filePath;
    _UnloadRecord(*it);
    m_LoadedScenes.erase(it);
    _ReindexScenes();
}

void SceneManager::UnloadScene(const SceneRecord *rec)
{
    if (!rec)
        return;
    UnloadScene(rec->filePath);
}

void SceneManager::UnloadSceneByName(const std::string &name)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(),
                           [&](const SceneRecord &r)
                           { return r.name == name; });

    if (it == m_LoadedScenes.end())
    {
        LOGGER_WARN("SceneManager") << "Scene not found by name: " << name;
        return;
    }
    if (it->inviolable)
    {
        LOGGER_WARN("SceneManager") << "Cannot unload inviolable scene: " << name;
        return;
    }
    LOGGER_INFO("SceneManager") << "Unloading scene by name: " << name;
    _UnloadRecord(*it);
    m_LoadedScenes.erase(it);
    _ReindexScenes();
}

void SceneManager::UnloadSceneByOrder(int order)
{
    auto it = std::find_if(m_LoadedScenes.begin(), m_LoadedScenes.end(),
                           [&](const SceneRecord &r)
                           { return r.loadOrder == order; });

    if (it == m_LoadedScenes.end())
    {
        LOGGER_WARN("SceneManager") << "Scene not found by order: " << order;
        return;
    }
    if (it->inviolable)
    {
        LOGGER_WARN("SceneManager") << "Cannot unload inviolable scene: order=" << order;
        return;
    }
    LOGGER_INFO("SceneManager") << "Unloading scene by order=" << order << ": " << it->name;
    _UnloadRecord(*it);
    m_LoadedScenes.erase(it);
    _ReindexScenes();
}

void SceneManager::PopScene()
{
    SceneRecord *best = nullptr;
    for (auto &rec : m_LoadedScenes)
    {
        if (rec.persistent || rec.inviolable)
            continue;
        if (!best || rec.loadOrder > best->loadOrder)
            best = &rec;
    }

    if (!best)
    {
        LOGGER_WARN("SceneManager") << "PopScene: No non-persistent scene to pop.";
        return;
    }

    LOGGER_INFO("SceneManager") << "PopScene: Unloading '" << best->name << "' (order=" << best->loadOrder << ")";
    std::string path = best->filePath;
    UnloadScene(path);
}

void SceneManager::ChangeScene(const std::string &filePath)
{
    for (int i = (int)m_LoadedScenes.size() - 1; i >= 0; --i)
    {
        if (!m_LoadedScenes[i].persistent)
        {
            _UnloadRecord(m_LoadedScenes[i]);
            m_LoadedScenes.erase(m_LoadedScenes.begin() + i);
        }
    }
    LoadScene(filePath);
    _ReindexScenes();
}

void SceneManager::ClearAllScenes()
{
    LOGGER_INFO("SceneManager") << "Clearing all non-persistent scenes...";
    m_Physics.Clear();
    for (int i = (int)m_LoadedScenes.size() - 1; i >= 0; --i)
    {
        if (!m_LoadedScenes[i].persistent && !m_LoadedScenes[i].inviolable)
        {
            _DestroySceneEntities(m_LoadedScenes[i]);
            _UnloadOrphanedResources(m_LoadedScenes[i]);
            m_LoadedScenes.erase(m_LoadedScenes.begin() + i);
        }
    }
    _ReindexScenes();
}

void SceneManager::ClearAllIncludingPersistent()
{
    LOGGER_INFO("SceneManager") << "Clearing all scenes (including persistent, except inviolable)...";
    m_Physics.Clear();
    for (int i = (int)m_LoadedScenes.size() - 1; i >= 0; --i)
    {
        if (!m_LoadedScenes[i].inviolable)
        {
            _DestroySceneEntities(m_LoadedScenes[i]);
            _UnloadOrphanedResources(m_LoadedScenes[i]);
            m_LoadedScenes.erase(m_LoadedScenes.begin() + i);
        }
    }
    _ReindexScenes();
}

void SceneManager::Shutdown()
{
    LOGGER_INFO("SceneManager") << "Shutting down and clearing ALL scenes...";
    m_Physics.Clear();
    for (int i = (int)m_LoadedScenes.size() - 1; i >= 0; --i)
    {
        _DestroySceneEntities(m_LoadedScenes[i]);
        _UnloadOrphanedResources(m_LoadedScenes[i]);
    }
    m_LoadedScenes.clear();
}

void SceneManager::QueueLoadScene(const std::string &path, bool persistent)
{
    m_pendingQueue.push_back({PendingOp::Load, path, persistent});
    LOGGER_INFO("SceneManager") << "Queued load scene: " << path;
}

void SceneManager::QueueUnloadScene(const std::string &path)
{
    const SceneRecord *rec = GetScene(path);
    if (!rec)
    {
        LOGGER_WARN("SceneManager") << "QueueUnloadScene: Scene not found or not loaded: " << path;
        return;
    }
    if (rec->inviolable)
    {
        LOGGER_WARN("SceneManager") << "QueueUnloadScene: Cannot queue unload for inviolable scene '" << rec->name << "'.";
        return;
    }
    m_pendingQueue.push_back({PendingOp::Unload, path, false});
    LOGGER_INFO("SceneManager") << "Queued unload scene: " << path;
}

void SceneManager::QueueChangeScene(const std::string &path)
{
    m_pendingQueue.push_back({PendingOp::Change, path, false});
    LOGGER_INFO("SceneManager") << "Queued change scene: " << path;
}

void SceneManager::QueuePopScene()
{
    m_pendingQueue.push_back({PendingOp::Pop, "", false});
    LOGGER_INFO("SceneManager") << "Queued pop scene.";
}

void SceneManager::UpdatePendingScene()
{
    if (m_pendingQueue.empty())
        return;
    std::vector<PendingOp> ops = std::move(m_pendingQueue);
    m_pendingQueue.clear();
    for (auto &op : ops)
    {
        switch (op.type)
        {
        case PendingOp::Load:
            LoadScene(op.path, op.persistent);
            break;
        case PendingOp::Unload:
            UnloadScene(op.path);
            break;
        case PendingOp::Change:
            ChangeScene(op.path);
            break;
        case PendingOp::Pop:
            PopScene();
            break;
        }
    }
}

void SceneManager::_ReindexScenes()
{
    std::sort(m_LoadedScenes.begin(), m_LoadedScenes.end(), [](const SceneRecord &a, const SceneRecord &b)
              { return a.loadOrder < b.loadOrder; });

    int index = 0;
    for (auto &rec : m_LoadedScenes)
    {
        rec.loadOrder = index++;
    }
    m_nextLoadOrder = index;

    LOGGER_INFO("SceneManager") << "Scenes re-indexed. Total active scenes: " << m_LoadedScenes.size();
}

std::vector<const SceneRecord *> SceneManager::GetScenes() const
{
    std::vector<const SceneRecord *> result;
    result.reserve(m_LoadedScenes.size());
    for (const auto &rec : m_LoadedScenes)
        result.push_back(&rec);
    return result;
}

bool SceneManager::IsLoaded(const std::string &filePath) const
{
    return std::any_of(m_LoadedScenes.begin(), m_LoadedScenes.end(),
                       [&](const SceneRecord &r)
                       { return r.filePath == filePath; });
}

const SceneRecord *SceneManager::GetScene(const std::string &filePath) const
{
    for (const auto &rec : m_LoadedScenes)
        if (rec.filePath == filePath)
            return &rec;
    return nullptr;
}

const SceneRecord *SceneManager::GetSceneByName(const std::string &name) const
{
    for (const auto &rec : m_LoadedScenes)
        if (rec.name == name)
            return &rec;
    return nullptr;
}

const SceneRecord *SceneManager::GetSceneByOrder(int order) const
{
    for (const auto &rec : m_LoadedScenes)
        if (rec.loadOrder == order)
            return &rec;
    return nullptr;
}

void SceneManager::LogScene(const std::string &filePath) const
{
    const SceneRecord *rec = GetScene(filePath);
    if (!rec)
    {
        LOGGER_WARN("SceneManager") << "LogScene: scene not found: " << filePath;
        return;
    }
    LOGGER_INFO("SceneManager") << "Scene '" << rec->name << "':"
                                << " path=" << rec->filePath
                                << " order=" << rec->loadOrder
                                << " persistent=" << rec->persistent
                                << " entities=" << rec->entities.size()
                                << " shaders=" << rec->ownedShaders.size()
                                << " models=" << rec->ownedModels.size()
                                << " textures=" << rec->ownedTextures.size()
                                << " fonts=" << rec->ownedFonts.size()
                                << " skyboxes=" << rec->ownedSkyboxes.size()
                                << " anims=" << rec->ownedAnimations.size()
                                << " sounds=" << rec->ownedSounds.size();
}

void SceneManager::LogAllScenes() const
{
    LOGGER_INFO("SceneManager") << "Loaded scenes (" << m_LoadedScenes.size() << "):";
    for (const auto &rec : m_LoadedScenes)
        LogScene(rec.filePath);
}
