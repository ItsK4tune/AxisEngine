#pragma once

#include <render/type/graphics_types.h>

class IRenderStateManager
{
public:
    virtual ~IRenderStateManager() = default;

    virtual void Enable(ServerCapability cap) = 0;
    virtual void Disable(ServerCapability cap) = 0;

    virtual void SetBlendFunc(BlendFactor sfactor, BlendFactor dfactor) = 0;
    virtual void SetBlendEquation(BlendEquation mode) = 0;

    virtual void SetDepthFunc(CompareFunc func) = 0;
    virtual void SetDepthMask(bool flag) = 0;

    virtual void SetStencilFunc(CompareFunc func, int ref, unsigned int mask) = 0;
    virtual void SetStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass) = 0;
    virtual void SetStencilMask(unsigned int mask) = 0;

    virtual void SetCullFace(CullMode mode) = 0;
    virtual void SetFrontFace(FrontFace mode) = 0;

    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetScissor(int x, int y, int width, int height) = 0;

    virtual void SetPolygonMode(CullMode face, PolygonMode mode) = 0;
    virtual PolygonMode GetPolygonMode() const = 0;
    virtual void SetLineWidth(float width) = 0;
    virtual void SetPointSize(float size) = 0;

    virtual void SetColorMask(bool r, bool g, bool b, bool a) = 0;

    virtual const char *GetBackendName() const = 0;
};
