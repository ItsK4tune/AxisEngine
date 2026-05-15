#pragma once

#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <render/logic/post_process_pipeline.h>
#include <memory>
#include <core/logic/logger.h>

struct Scene;

class PostProcessSystem : public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override;
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 95; }
    std::string GetName() const override { return "PostProcessSystem"; }
    SystemCategory GetCategory() const override { return SystemCategory::PostProcess | SystemCategory::RenderCapture | SystemCategory::RenderUI; }
    SystemRequirement GetRequirements() const override { return SystemRequirement::Graphics; }
    
    void Render(Scene &scene) override;
    void RenderCapturePass(Scene &scene, int width, int height) override;
    void RenderAlphaPass(Scene &scene, int width, int height, float alpha) override;
    void RenderUIPass(Scene &scene, float width, float height, IRenderStateManager &renderState) override;
    
    PostProcessPipeline& GetPipeline() { return m_Pipeline; }
    uint32_t GetCaptureFBO() { return m_Pipeline.GetCaptureFBO(); }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    PostProcessPipeline m_Pipeline;
    class IRenderService* m_RenderService = nullptr;
};
