#pragma once

#include <platform/interface/i_window.h>
#include <render/type/graphics_types.h>
#include <string>

class IBufferManager;
class IDrawContext;
class IQueryManager;
class IRenderStateManager;
class IRenderTargetManager;
class IShaderManager;
class ITextureManager;
struct Scene;

namespace rhi
{
class IRenderBackend;
}

class IGraphicsContext
{
public:
    virtual ~IGraphicsContext() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void SetWindow(IWindow* window)
    {
        (void)window;
    }

    virtual bool BeginFrame()
    {
        return true;
    }

    virtual void EndFrame()
    {
    }

    virtual bool SupportsLegacyRenderPipeline() const
    {
        return true;
    }

    virtual bool RenderNativeScene(Scene& scene, int width, int height, float alpha)
    {
        (void)scene;
        (void)width;
        (void)height;
        (void)alpha;
        return false;
    }

    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetDepthTest(bool enabled) = 0;
    virtual void SetCullFace(bool enabled) = 0;
    virtual void SetBlending(bool enabled) = 0;
    virtual void SetBlendFunc(BlendFactor src, BlendFactor dst) = 0;

    virtual void Clear(BufferBit flags) = 0;

    virtual IBufferManager& GetBufferManager() = 0;
    virtual ITextureManager& GetTextureManager() = 0;
    virtual IShaderManager& GetShaderManager() = 0;
    virtual IRenderTargetManager& GetRenderTargetManager() = 0;
    virtual IRenderStateManager& GetRenderStateManager() = 0;
    virtual IDrawContext& GetDrawContext() = 0;
    virtual IQueryManager& GetQueryManager() = 0;
    virtual rhi::IRenderBackend* GetRenderBackend()
    {
        return nullptr;
    }

    virtual bool TryGetVramUsage(uint64_t& usedBytes, uint64_t& totalBytes) const
    {
        (void)usedBytes;
        (void)totalBytes;
        return false;
    }

    virtual std::string GetName() const = 0;

    virtual std::string GetDeviceName() const
    {
        return GetName();
    }
};
