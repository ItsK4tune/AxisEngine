#include <ecs/system.h>
#include <ecs/component.h>
#include <graphic/geometry/animator.h>
#include <utils/logger.h>

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
