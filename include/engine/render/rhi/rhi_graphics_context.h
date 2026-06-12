#pragma once

#include <core/type/app_config.h>
#include <render/interface/i_graphics_context.h>
#include <render/rhi/i_render_backend.h>
#include <render/rhi/rhi_frame_renderer.h>
#include <render/rhi/rhi_scene_renderer.h>
#include <memory>

class RhiGraphicsContext final : public IGraphicsContext
{
public:
    explicit RhiGraphicsContext(const AppConfig& config);
    ~RhiGraphicsContext() override;

    void SetWindow(IWindow* window) override;
    bool Initialize() override;
    void Shutdown() override;
    bool BeginFrame() override;
    void EndFrame() override;
    bool SupportsLegacyRenderPipeline() const override;
    bool RenderNativeScene(Scene& scene, int width, int height, float alpha) override;

    void SetViewport(int x, int y, int width, int height) override;
    void SetDepthTest(bool enabled) override;
    void SetCullFace(bool enabled) override;
    void SetBlending(bool enabled) override;
    void SetBlendFunc(BlendFactor src, BlendFactor dst) override;
    void Clear(BufferBit flags) override;

    IBufferManager& GetBufferManager() override;
    ITextureManager& GetTextureManager() override;
    IShaderManager& GetShaderManager() override;
    IRenderTargetManager& GetRenderTargetManager() override;
    IRenderStateManager& GetRenderStateManager() override;
    IDrawContext& GetDrawContext() override;
    IQueryManager& GetQueryManager() override;
    rhi::IRenderBackend* GetRenderBackend() override;

    std::string GetName() const override;

private:
    void ClearBackBuffer();

    AppConfig m_Config;
    IWindow* m_Window = nullptr;
    std::unique_ptr<rhi::IRenderBackend> m_Backend;
    std::unique_ptr<RhiFrameRenderer> m_FrameRenderer;
    std::unique_ptr<RhiSceneRenderer> m_SceneRenderer;
    int m_ViewportWidth = 0;
    int m_ViewportHeight = 0;
    bool m_FrameActive = false;
};
