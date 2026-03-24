#pragma once

#include <ecs/interface/i_lighting_service.h>
#include <ecs/interface/i_ecs_system.h>
#include <render/logic/light_renderer.h>
#include <memory>

class LightingSystem : public IRenderSystem, public IECSSystem, public ILightingService
{
public:
    void Initialize() override;
    void Shutdown() override;
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 83; }
    std::string GetName() const override { return "LightingSystem"; }
    void Render(Scene &scene) override {}
    void RenderAlpha(Scene &scene, int width, int height, float alpha) override;
    
    void RenderDeferredLighting(Scene &scene, int width, int height);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    LightRenderer m_LightRenderer;
    std::shared_ptr<Shader> m_DeferredLightShader;
    
    GpuHandle m_QuadVAO = 0;
    GpuHandle m_QuadVBO = 0;
    void InitQuad();

    class IGraphicsContext* m_GraphicsContext = nullptr;
    class IGeometryService* m_GeoService = nullptr;
    class IShadowService* m_ShadowService = nullptr;
    class IRenderService* m_RenderService = nullptr;
};
