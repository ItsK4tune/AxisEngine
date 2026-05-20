#pragma once

#include <render/type/graphics_types.h>
#include <string>

class IBufferManager;
class IDrawContext;
class IQueryManager;
class IRenderStateManager;
class IRenderTargetManager;
class IShaderManager;
class ITextureManager;

class IGraphicsContext
{
public:
    virtual ~IGraphicsContext() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

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

    virtual std::string GetName() const = 0;
};
