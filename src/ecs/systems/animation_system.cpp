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
            anim.animator->UpdateAnimation(dt * anim.speed);
        }
    }
}
