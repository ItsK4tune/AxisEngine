#pragma once

#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <scene/logic/scene.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <cstddef>
#include <memory>
#include <vector>

struct AnimationSnapshotItem
{
    entt::entity entity = entt::null;
    std::shared_ptr<Animator> animator = nullptr;
    float speed = 1.0f;
    glm::mat4 rootTransform = glm::mat4(1.0f);
};

class FrameSnapshot
{
public:
    void CaptureAnimations(Scene& scene)
    {
        m_Animations.clear();

        auto view = scene.registry.view<AnimationComponent, MeshRendererComponent, InfoComponent>();
        m_Animations.reserve(view.size_hint());

        for (auto entity : view)
        {
            const auto& info = view.get<InfoComponent>(entity);
            if (!info.isActive)
                continue;

            const auto& animation = view.get<AnimationComponent>(entity);
            if (!animation.animator)
                continue;

            glm::mat4 rootTransform = glm::mat4(1.0f);
            const auto& renderer = view.get<MeshRendererComponent>(entity);
            if (renderer.model)
            {
                rootTransform = renderer.model->GetRootTransform();
            }

            m_Animations.push_back({entity, animation.animator, animation.speed, rootTransform});
        }
    }

    const std::vector<AnimationSnapshotItem>& Animations() const
    {
        return m_Animations;
    }

    void Clear()
    {
        m_Animations.clear();
    }

private:
    std::vector<AnimationSnapshotItem> m_Animations;
};
