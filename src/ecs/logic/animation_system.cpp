#include <ecs/logic/animation_system.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/animator.h>
#include <algorithm>

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

    auto view = scene.View<AnimationComponent, MeshRendererComponent, InfoComponent>();

    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto& anim = view.get<AnimationComponent>(entity);
        if (anim.animator)
        {
            glm::mat4 rootTransform = glm::mat4(1.0f);
            if (scene.HasAllComponents<MeshRendererComponent>(entity))
            {
                auto& renderer = scene.GetComponent<MeshRendererComponent>(entity);
                if (renderer.model)
                {
                    rootTransform = renderer.model->GetRootTransform();
                }
            }
            anim.animator->UpdateAnimation(dt * anim.speed, rootTransform);
        }
    }
}

void AnimationSystem::CaptureSnapshot(Scene& scene, FrameSnapshot& snapshot)
{
    snapshot.CaptureAnimations(scene);
}

void AnimationSystem::UpdateParallel(const FrameSnapshot& snapshot, ECSCommandBuffer& commands, float dt)
{
    (void)commands;

    const auto& animations = snapshot.Animations();
    if (animations.empty())
        return;

    auto updateRange = [&animations, dt](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i)
        {
            const auto& item = animations[i];
            if (item.animator)
            {
                item.animator->UpdateAnimation(dt * item.speed, item.rootTransform);
            }
        }
    };

    constexpr size_t kParallelThreshold = 64;
    if (animations.size() < kParallelThreshold)
    {
        updateRange(0, animations.size());
        return;
    }

    const uint32_t workerCount = JobSystem::Instance().GetThreadCount();
    if (workerCount <= 1)
    {
        updateRange(0, animations.size());
        return;
    }

    const size_t jobCount = std::min<size_t>(animations.size(), workerCount);
    const size_t chunkSize = (animations.size() + jobCount - 1) / jobCount;

    JobSystem::JobCounter counter{0};
    for (size_t job = 0; job < jobCount; ++job)
    {
        const size_t begin = job * chunkSize;
        const size_t end = std::min(animations.size(), begin + chunkSize);
        if (begin >= end)
            break;

        JobSystem::Instance().Execute([updateRange, begin, end]() { updateRange(begin, end); }, &counter);
    }
    JobSystem::Instance().Wait(&counter);
}

std::vector<entt::id_type> AnimationSystem::GetReadComponents() const
{
    return {entt::type_id<MeshRendererComponent>().hash()};
}

std::vector<entt::id_type> AnimationSystem::GetWriteComponents() const
{
    return {entt::type_id<AnimationComponent>().hash()};
}
