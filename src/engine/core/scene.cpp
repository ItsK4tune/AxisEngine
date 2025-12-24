#include <engine/core/scene.h>

entt::entity Scene::createEntity()
{
    return registry.create();
}

void Scene::destroyEntity(entt::entity entity)
{
    registry.destroy(entity);
}

entt::entity Scene::GetActiveCamera()
{
    auto view = registry.view<const CameraComponent>();
    for (auto entity : view)
    {
        const auto &cam = view.get<const CameraComponent>(entity);
        if (cam.isPrimary)
        {
            return entity;
        }
    }
    return entt::null;
}