#include <core/scene_manager.h>
#include <core/scene_loader.h>
#include <core/script_registry.h>
#include <utils/filesystem.h>
#include <utils/filesystem.h>
#include <utils/bullet_glm_helpers.h>
#include <core/application.h>
#include <map>

SceneManager::SceneManager(Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound, Application *app)
    : m_Scene(scene), m_Resources(res), m_Physics(phys), m_SoundManager(sound), m_App(app) {}

void SceneManager::LoadScene(const std::string &filePath)
{
    if (m_LoadedScenes.find(filePath) != m_LoadedScenes.end())
    {
        std::cout << "[SceneManager] Scene already loaded: " << filePath << std::endl;
        return;
    }

    std::vector<entt::entity> loadedEntities = SceneLoader::Load(filePath, m_Scene, m_Resources, m_Physics, m_SoundManager, m_App);
    
    if (!loadedEntities.empty())
    {
        m_LoadedScenes[filePath] = loadedEntities;
    }
}

void SceneManager::UnloadScene(const std::string &filePath)
{
    auto it = m_LoadedScenes.find(filePath);
    if (it == m_LoadedScenes.end())
    {
        std::cout << "[SceneManager] Scene not found or not loaded: " << filePath << std::endl;
        return;
    }

    for (auto entity : it->second)
    {
        m_Scene.destroyEntity(entity, this);
    }

    m_LoadedScenes.erase(it);
}

void SceneManager::ChangeScene(const std::string &filePath)
{
    ClearAllScenes();
    LoadScene(filePath);
}

void SceneManager::ClearAllScenes()
{
    m_Scene.registry.clear();

    m_Physics.Clear();

    m_LoadedScenes.clear();
}

void SceneManager::QueueLoadScene(const std::string &path)
{
    m_pendingPath = path;
    m_isPending = true;
    std::cout << "[SceneManager] Queued load scene: " << path << std::endl;
}

void SceneManager::UpdatePendingScene()
{
    if (m_isPending)
    {
        ClearAllScenes();
        LoadScene(m_pendingPath);
        m_isPending = false;
        m_pendingPath = "";
    }
}