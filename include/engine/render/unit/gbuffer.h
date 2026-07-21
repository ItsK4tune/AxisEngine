#pragma once

#include <render/type/graphics_types.h>
#include <memory>
#include <vector>

class IGraphicsContext;

class GBuffer
{
public:
    GBuffer();
    ~GBuffer();

    void Initialize(IGraphicsContext& context, int width, int height);
    void Shutdown();
    void Resize(int width, int height);

    void BindForWriting();
    void Unbind();
    void Resolve();

    uint32_t GetNormalTexture() const
    {
        return m_NormalTexture ? m_NormalTexture->Get() : 0;
    }
    uint32_t GetAlbedoSpecTexture() const
    {
        return m_AlbedoSpecTexture ? m_AlbedoSpecTexture->Get() : 0;
    }
    uint32_t GetIDTexture() const
    {
        return m_IDTexture ? m_IDTexture->Get() : 0;
    }
    uint32_t GetEmissiveTexture() const
    {
        return m_EmissiveTexture ? m_EmissiveTexture->Get() : 0;
    }
    uint32_t GetPBRParamsTexture() const
    {
        return m_PBRParamsTexture ? m_PBRParamsTexture->Get() : 0;
    }
    uint32_t GetDepthTexture() const
    {
        return m_DepthTexture ? m_DepthTexture->Get() : 0;
    }
    uint32_t GetFBO() const
    {
        return m_FBO ? m_FBO->Get() : 0;
    }
    int GetWidth() const
    {
        return m_Width;
    }
    int GetHeight() const
    {
        return m_Height;
    }
    int GetScaledWidth() const
    {
        return (int)(m_Width * m_RenderScale);
    }
    int GetScaledHeight() const
    {
        return (int)(m_Height * m_RenderScale);
    }

    void SetRenderScale(float scale)
    {
        m_RenderScale = scale > 0.0f ? scale : 1.0f;
    }
    float GetRenderScale() const
    {
        return m_RenderScale;
    }
    void SetSampleCount(int samples)
    {
        m_RequestedSampleCount = samples > 1 ? samples : 1;
        m_SampleCount = m_RequestedSampleCount;
    }
    int GetSampleCount() const
    {
        return m_RequestedSampleCount;
    }
    int GetActiveSampleCount() const { return m_SampleCount; }
    void SetEntityIdEnabled(bool enabled) { m_EntityIdEnabled = enabled; }
    bool IsEntityIdEnabled() const { return m_EntityIdEnabled; }

private:
    IGraphicsContext* m_Context = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    float m_RenderScale = 1.0f;
    float m_AllocatedRenderScale = 0.0f;
    int m_SampleCount = 1;
    int m_RequestedSampleCount = 1;
    int m_AllocatedSampleCount = 0;
    bool m_EntityIdEnabled = true;
    bool m_AllocatedEntityIdEnabled = false;

    std::unique_ptr<GPUFramebuffer> m_FBO;
    std::unique_ptr<GPUTexture> m_NormalTexture;
    std::unique_ptr<GPUTexture> m_AlbedoSpecTexture;
    std::unique_ptr<GPUTexture> m_IDTexture;
    std::unique_ptr<GPUTexture> m_EmissiveTexture;
    std::unique_ptr<GPUTexture> m_PBRParamsTexture;
    std::unique_ptr<GPUTexture> m_DepthTexture;

    std::unique_ptr<GPUFramebuffer> m_MultisampleFBO;
    std::vector<std::unique_ptr<GPUTexture>> m_MultisampleColorTextures;
    std::unique_ptr<GPUTexture> m_MultisampleDepthTexture;

    void CreateTextures();
    void CreateMultisampleTargets();
};
