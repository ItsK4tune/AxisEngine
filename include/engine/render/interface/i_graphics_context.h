#pragma once

#include <render/type/graphics_types.h>
#include <cstdint>
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

    // Call after an external renderer (for example ImGui or a plugin issuing
    // native graphics calls) may have changed bindings or fixed-function state.
    virtual void InvalidateStateCache() {}
    virtual void SetStateCacheEnabled(bool) {}

    virtual IBufferManager& GetBufferManager() = 0;
    virtual ITextureManager& GetTextureManager() = 0;
    virtual IShaderManager& GetShaderManager() = 0;
    virtual IRenderTargetManager& GetRenderTargetManager() = 0;
    virtual IRenderStateManager& GetRenderStateManager() = 0;
    virtual IDrawContext& GetDrawContext() = 0;
    virtual IQueryManager& GetQueryManager() = 0;

    virtual std::string GetName() const = 0;
    virtual bool TryGetMemoryBudget(uint64_t&, uint64_t&) const
    {
        return false;
    }
};
