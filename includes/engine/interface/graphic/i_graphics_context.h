#pragma once
#include <interface/graphic/graphics_types.h>
#include <string>

class IBufferManager;
class ITextureManager;
class IShaderManager;
class IRenderTargetManager;
class IRenderStateManager;
class IDrawContext;
class IQueryManager;

class IGraphicsContext
{
public:
    virtual ~IGraphicsContext() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;

    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetDepthTest(bool enabled) = 0;
    virtual void SetCullFace(bool enabled) = 0;
    virtual void SetBlending(bool enabled) = 0;
    virtual void SetBlendFunc(Graphics::BlendFactor src, Graphics::BlendFactor dst) = 0;

    virtual void Clear(Graphics::BufferBit flags) = 0;

    virtual IBufferManager &GetBufferManager() = 0;
    virtual ITextureManager &GetTextureManager() = 0;
    virtual IShaderManager &GetShaderManager() = 0;
    virtual IRenderTargetManager &GetRenderTargetManager() = 0;
    virtual IRenderStateManager &GetRenderStateManager() = 0;
    virtual IDrawContext &GetDrawContext() = 0;
    virtual IQueryManager &GetQueryManager() = 0;

    virtual std::string GetName() const = 0;
};
