#include <ecs/system.h>
#include <execution>
#include <ecs/component.h>

void AnimationSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled) return;

    auto view = scene.registry.view<AnimationComponent>();

    std::vector<entt::entity> entities(view.begin(), view.end());

    std::for_each(std::execution::par, entities.begin(), entities.end(), [&scene, dt](entt::entity entity)
                  {
        auto &anim = scene.registry.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            anim.animator->UpdateAnimation(dt);
        } });
}
