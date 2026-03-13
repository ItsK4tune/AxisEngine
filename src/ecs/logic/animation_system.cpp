#include <ecs/unit/media_components.h>
#include <ecs/logic/animation_system.h>
#include <render/logic/animator.h>
#include <ecs/unit/render_components.h>
#include <core/logic/logger.h>

void AnimationSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled) return;

    auto view = scene.registry.view<AnimationComponent, MeshRendererComponent>();

    for (auto entity : view)
    {
        auto &anim = scene.registry.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            glm::mat4 rootTransform = glm::mat4(1.0f);
            if (scene.registry.all_of<MeshRendererComponent>(entity))
            {
                auto &renderer = scene.registry.get<MeshRendererComponent>(entity);
                if (renderer.model)
                {
                    rootTransform = renderer.model->GetRootTransform();
                }
            }
            anim.animator->UpdateAnimation(dt * anim.speed, rootTransform);
        }
    }
}

std::vector<entt::id_type> AnimationSystem::GetReadComponents() const
{
    return {entt::type_id<MeshRendererComponent>().hash()};
}

std::vector<entt::id_type> AnimationSystem::GetWriteComponents() const
{
    return {entt::type_id<AnimationComponent>().hash()};
}
