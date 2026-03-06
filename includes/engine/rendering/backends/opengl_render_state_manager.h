#pragma once

#include <rendering/interfaces/i_render_state_manager.h>
#include <unordered_map>

class OpenGLRenderStateManager : public IRenderStateManager
{
public:
    void Enable(Graphics::ServerCapability cap) override;
    void Disable(Graphics::ServerCapability cap) override;

    void BlendFunc(Graphics::BlendFactor sfactor, Graphics::BlendFactor dfactor) override;
    void BlendEquation(Graphics::BlendEquation mode) override;

    void DepthFunc(Graphics::CompareFunc func) override;
    void DepthMask(bool flag) override;

    void StencilFunc(Graphics::CompareFunc func, int ref, unsigned int mask) override;
    void StencilOp(Graphics::StencilOp sfail, Graphics::StencilOp dpfail, Graphics::StencilOp dppass) override;
    void StencilMask(unsigned int mask) override;

    void CullFace(Graphics::CullMode mode) override;
    void FrontFace(Graphics::FrontFace mode) override;

    void Viewport(int x, int y, int width, int height) override;
    void Scissor(int x, int y, int width, int height) override;

    void PolygonMode(Graphics::CullMode face, Graphics::PolygonMode mode) override;
    Graphics::PolygonMode GetPolygonMode() const override;

    void LineWidth(float width) override;
    void PointSize(float size) override;

    void ColorMask(bool r, bool g, bool b, bool a) override;

    const char *GetBackendName() const override;

private:
    std::unordered_map<Graphics::ServerCapability, bool> m_Capabilities;

    Graphics::BlendFactor m_BlendSrc = Graphics::BlendFactor::One;
    Graphics::BlendFactor m_BlendDst = Graphics::BlendFactor::Zero;
    Graphics::BlendEquation m_BlendEquation = Graphics::BlendEquation::Add;

    Graphics::CompareFunc m_DepthFunc = Graphics::CompareFunc::Less;
    bool m_DepthMask = true;

    Graphics::CompareFunc m_StencilFunc = Graphics::CompareFunc::Always;
    int m_StencilRef = 0;
    unsigned int m_StencilMask = 0xFFFFFFFF;
    Graphics::StencilOp m_StencilFail = Graphics::StencilOp::Keep;
    Graphics::StencilOp m_StencilDepthFail = Graphics::StencilOp::Keep;
    Graphics::StencilOp m_StencilPass = Graphics::StencilOp::Keep;
    unsigned int m_StencilWriteMask = 0xFFFFFFFF;

    Graphics::CullMode m_CullMode = Graphics::CullMode::Back;
    Graphics::FrontFace m_FrontFace = Graphics::FrontFace::CCW;

    int m_ViewportX = 0, m_ViewportY = 0, m_ViewportW = 0, m_ViewportH = 0;
    int m_ScissorX = 0, m_ScissorY = 0, m_ScissorW = 0, m_ScissorH = 0;

    Graphics::CullMode m_PolygonFace = Graphics::CullMode::FrontAndBack;
    Graphics::PolygonMode m_PolygonMode = Graphics::PolygonMode::Fill;

    float m_LineWidth = 1.0f;
    float m_PointSize = 1.0f;

    bool m_ColorMaskR = true, m_ColorMaskG = true, m_ColorMaskB = true, m_ColorMaskA = true;
};
