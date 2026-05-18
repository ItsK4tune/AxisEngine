#include <ecs/logic/animation_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/animator.h>

void AnimationSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<AnimationSystem>(this);
}

REGISTER_SYSTEM(AnimationSystem)

void AnimationSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    auto view = scene.registry.view<AnimationComponent, MeshRendererComponent, InfoComponent>();

    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto& anim = view.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            glm::mat4 rootTransform = glm::mat4(1.0f);
            if (scene.registry.all_of<MeshRendererComponent>(entity))
            {
                auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
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
