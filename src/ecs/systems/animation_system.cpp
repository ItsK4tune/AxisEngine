#include <ecs/system.h>
#include <execution>
#include <ecs/component.h>

void AnimationSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled) return;

    auto view = scene.registry.view<AnimationComponent>();

    m_Entities.assign(view.begin(), view.end());

    std::for_each(std::execution::par, m_Entities.begin(), m_Entities.end(), [&scene, dt](entt::entity entity)
                  {
        auto &anim = scene.registry.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            anim.animator->UpdateAnimation(dt);
        } });
}
