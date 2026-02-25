#include <ecs/entity_manager.h>
#include <ecs/components/info_component.h>

void EntityManager::Destroy(Scene& scene, entt::entity entity)
{
    if (entity != entt::null && scene.registry.valid(entity))
    {
        scene.destroyEntity(entity);
    }
}

entt::entity EntityManager::FindByName(Scene& scene, const std::string& name)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::FindByTag(Scene& scene, const std::string& tag)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::FindByNameAndTag(Scene& scene, const std::string& name, const std::string& tag)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag)
            return entity;
    }
    return entt::null;
}

entt::entity EntityManager::FindByNameTagAndScene(Scene& scene, const std::string& name, const std::string& tag, const std::string& sceneName)
{
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        if (info.name == name && info.tag == tag && info.sceneName == sceneName)
            return entity;
    }
    return entt::null;
}

std::vector<entt::entity> EntityManager::FindAllByName(Scene& scene, const std::string& name)
{
    std::vector<entt::entity> results;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            results.push_back(entity);
    }
    return results;
}

std::vector<entt::entity> EntityManager::FindAllByTag(Scene& scene, const std::string& tag)
{
    std::vector<entt::entity> results;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).tag == tag)
            results.push_back(entity);
    }
    return results;
}

std::vector<entt::entity> EntityManager::FindAllBySceneName(Scene& scene, const std::string& sceneName)
{
    std::vector<entt::entity> results;
    auto view = scene.registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).sceneName == sceneName)
            results.push_back(entity);
    }
    return results;
}
