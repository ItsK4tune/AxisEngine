#pragma once

#include <render/interface/i_render_state_manager.h>
#include <unordered_map>

class OpenGLRenderStateManager : public IRenderStateManager
{
public:
    void Enable(ServerCapability cap) override;
    void Disable(ServerCapability cap) override;

    void SetBlendFunc(BlendFactor sfactor, BlendFactor dfactor) override;
    void SetBlendEquation(BlendEquation mode) override;

    void SetDepthFunc(CompareFunc func) override;
    void SetDepthMask(bool flag) override;

    void SetStencilFunc(CompareFunc func, int ref, unsigned int mask) override;
    void SetStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass) override;
    void SetStencilMask(unsigned int mask) override;

    void SetCullFace(CullMode mode) override;
    void SetFrontFace(FrontFace mode) override;

    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;

    void SetPolygonMode(CullMode face, PolygonMode mode) override;
    PolygonMode GetPolygonMode() const override;

    void SetLineWidth(float width) override;
    void SetPointSize(float size) override;

    void SetColorMask(bool r, bool g, bool b, bool a) override;

    const char *GetBackendName() const override;

private:
    std::unordered_map<ServerCapability, bool> m_Capabilities;

    BlendFactor m_BlendSrc = BlendFactor::One;
    BlendFactor m_BlendDst = BlendFactor::Zero;
    BlendEquation m_BlendEquation = BlendEquation::Add;

    CompareFunc m_DepthFunc = CompareFunc::Less;
    bool m_DepthMask = true;

    CompareFunc m_StencilFunc = CompareFunc::Always;
    int m_StencilRef = 0;
    unsigned int m_StencilMask = 0xFFFFFFFF;
    StencilOp m_StencilFail = StencilOp::Keep;
    StencilOp m_StencilDepthFail = StencilOp::Keep;
    StencilOp m_StencilPass = StencilOp::Keep;
    unsigned int m_StencilWriteMask = 0xFFFFFFFF;

    CullMode m_CullMode = CullMode::Back;
    FrontFace m_FrontFace = FrontFace::CCW;

    int m_ViewportX = 0, m_ViewportY = 0, m_ViewportW = 0, m_ViewportH = 0;
    int m_ScissorX = 0, m_ScissorY = 0, m_ScissorW = 0, m_ScissorH = 0;

    CullMode m_PolygonFace = CullMode::FrontAndBack;
    PolygonMode m_PolygonMode = PolygonMode::Fill;

    float m_LineWidth = 1.0f;
    float m_PointSize = 1.0f;

    bool m_ColorMaskR = true, m_ColorMaskG = true, m_ColorMaskB = true, m_ColorMaskA = true;
};