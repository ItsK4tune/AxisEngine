#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_system.h>
#include <render/logic/material_renderer.h>
#include <render/unit/command_queue.h>
#include <render/unit/gbuffer.h>
#include <memory>

class GeometrySystem : public IRenderSystem, public IECSSystem, public IGeometryService
{
public:
    void Initialize() override;
    void Shutdown() override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    int GetPriority() const override
    {
        return 80;
    }
    std::string GetName() const override
    {
        return "GeometrySystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderMain;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void Render(Scene& scene) override;
    void RenderAlphaPass(Scene& scene, int width, int height, float alpha) override
    {
    }

    GBuffer& GetGBuffer() override
    {
        return m_GBuffer;
    }
    void BindGBufferForWriting() override;
    void UnbindGBuffer() override;

    void BeginDecalPass() override;
    void EndDecalPass(uint32_t mainFBO) override;

    bool IsDeferredRenderingEnabled() const override
    {
        return m_Enabled && m_IsDeferredCached;
    }

    uint32_t GetGBufferDepth() const override
    {
        return m_GBuffer.GetDepthTexture();
    }
    uint32_t GetGBufferID() const override
    {
        return m_GBuffer.GetIDTexture();
    }
    uint32_t GetGBufferPosition() const override
    {
        return m_GBuffer.GetPositionTexture();
    }
    uint32_t GetGBufferNormal() const override
    {
        return m_GBuffer.GetNormalTexture();
    }
    uint32_t GetGBufferWidth() const override
    {
        return (uint32_t)m_GBuffer.GetScaledWidth();
    }
    uint32_t GetGBufferHeight() const override
    {
        return (uint32_t)m_GBuffer.GetScaledHeight();
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    GBuffer m_GBuffer;
    std::shared_ptr<Shader> m_GBufferShader;
    CommandQueue m_CommandQueue;
    bool m_IsDeferredCached = false;

    class IRenderService* m_RenderService = nullptr;
    class IShadowService* m_ShadowService = nullptr;
    class IGraphicsContext* m_GraphicsContext = nullptr;
    class ConfigManager* m_ConfigManager = nullptr;
};
