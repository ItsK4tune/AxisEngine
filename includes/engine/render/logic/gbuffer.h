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
    void BindForReading();
    void Unbind();

    uint32_t GetPositionTexture() const { return m_PositionTexture ? m_PositionTexture->Get() : 0; }
    uint32_t GetNormalTexture() const { return m_NormalTexture ? m_NormalTexture->Get() : 0; }
    uint32_t GetAlbedoSpecTexture() const { return m_AlbedoSpecTexture ? m_AlbedoSpecTexture->Get() : 0; }
    uint32_t GetIDTexture() const { return m_IDTexture ? m_IDTexture->Get() : 0; }
    uint32_t GetDepthTexture() const { return m_DepthTexture ? m_DepthTexture->Get() : 0; }
    uint32_t GetFBO() const { return m_FBO ? m_FBO->Get() : 0; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    IGraphicsContext* m_Context = nullptr;
    int m_Width = 0;
    int m_Height = 0;

    std::unique_ptr<GPUFramebuffer> m_FBO;
    std::unique_ptr<GPUTexture> m_PositionTexture;
    std::unique_ptr<GPUTexture> m_NormalTexture;
    std::unique_ptr<GPUTexture> m_AlbedoSpecTexture;
    std::unique_ptr<GPUTexture> m_IDTexture;
    std::unique_ptr<GPUTexture> m_DepthTexture;

    void CreateTextures();
};
