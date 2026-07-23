#pragma once

#include <core/logic/logger.h>
#include <core/logic/event_manager.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <render/interface/i_post_process_registry.h>
#include <render/logic/post_process_pipeline.h>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>

struct Scene;

class PostProcessSystem : public IRenderSystem, public IECSSystem, public IPostProcessRegistry
{
public:
    void Initialize() override;
    void Shutdown() override;

    bool IsEnabled() const override
    {
        return m_EffectsEnabled;
    }
    void SetEnabled(bool enable) override
    {
        m_EffectsEnabled = enable;
    }
    bool AreEffectsEnabled() const
    {
        return m_EffectsEnabled;
    }
    void SetEffectsEnabled(bool enable)
    {
        m_EffectsEnabled = enable;
    }
    void SetPresentToBackbuffer(bool present) { m_PresentToBackbuffer = present; }
    int GetPriority() const override
    {
        return 95;
    }
    std::string GetName() const override
    {
        return "PostProcessSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::PostProcess | SystemCategory::RenderCapture | SystemCategory::RenderUI;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

    void Render(Scene& scene) override;
    void RenderCapturePass(Scene& scene, int width, int height) override;
    void RenderUIPass(Scene& scene, float width, float height, IRenderStateManager& renderState) override;

    PostProcessPipeline& GetPipeline()
    {
        return m_Pipeline;
    }
    uint32_t GetCaptureFBO()
    {
        return m_Pipeline.GetCaptureFBO();
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    PostProcessEffectHandle RegisterEffect(PostProcessEffectDescriptor descriptor) override;
    bool UnregisterEffect(PostProcessEffectHandle handle) override;
    size_t UnregisterOwner(std::string_view owner) override;
    std::vector<RegisteredPostProcessEffect> GetRegisteredEffects() const override;

private:
    bool m_EffectsEnabled = true;
    bool m_PresentToBackbuffer = true;
    PostProcessPipeline m_Pipeline;
    class IRenderService* m_RenderService = nullptr;
    int m_LastAntiAliasingMode = -1;
    EventSubscriptionList m_EventSubscriptions;
    mutable std::mutex m_RegistryMutex;
    std::unordered_map<PostProcessEffectHandle, PostProcessEffectDescriptor> m_RegisteredEffects;
    std::atomic<std::shared_ptr<const std::vector<RegisteredPostProcessEffect>>> m_RegisteredEffectsSnapshot{
        std::make_shared<const std::vector<RegisteredPostProcessEffect>>()};
    PostProcessEffectHandle m_NextEffectHandle = 1;

    void RebuildRegistrySnapshotLocked();
};
