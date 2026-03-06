#pragma once

#include <rendering/types/graphics_enums.h>
#include <rendering/types/buffer_types.h>
#include <rendering/types/texture_types.h>
#include <rendering/types/render_state_types.h>
#include <rendering/types/framebuffer_types.h>
#include <rendering/types/graphics_query_types.h>
#include <rendering/types/gpu_handle.h>

class IRenderStateManager
{
public:
    virtual ~IRenderStateManager() = default;

    virtual void Enable(Graphics::ServerCapability cap) = 0;
    virtual void Disable(Graphics::ServerCapability cap) = 0;

    virtual void BlendFunc(Graphics::BlendFactor sfactor, Graphics::BlendFactor dfactor) = 0;
    virtual void BlendEquation(Graphics::BlendEquation mode) = 0;

    virtual void DepthFunc(Graphics::CompareFunc func) = 0;
    virtual void DepthMask(bool flag) = 0;

    virtual void StencilFunc(Graphics::CompareFunc func, int ref, unsigned int mask) = 0;
    virtual void StencilOp(Graphics::StencilOp sfail, Graphics::StencilOp dpfail, Graphics::StencilOp dppass) = 0;
    virtual void StencilMask(unsigned int mask) = 0;

    virtual void CullFace(Graphics::CullMode mode) = 0;
    virtual void FrontFace(Graphics::FrontFace mode) = 0;

    virtual void Viewport(int x, int y, int width, int height) = 0;
    virtual void Scissor(int x, int y, int width, int height) = 0;

    virtual void PolygonMode(Graphics::CullMode face, Graphics::PolygonMode mode) = 0;
    virtual Graphics::PolygonMode GetPolygonMode() const = 0;
    virtual void LineWidth(float width) = 0;
    virtual void PointSize(float size) = 0;

    virtual void ColorMask(bool r, bool g, bool b, bool a) = 0;

    virtual const char *GetBackendName() const = 0;
};
