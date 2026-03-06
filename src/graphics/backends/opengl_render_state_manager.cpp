#include <graphics/backends/opengl_render_state_manager.h>
#include <glad/glad.h>
#include <graphics/backends/opengl_translator.h>

void OpenGLRenderStateManager::Enable(Graphics::ServerCapability cap)
{
    if (!m_Capabilities[cap])
    {
        m_Capabilities[cap] = true;
        glEnable(GLTranslator::ToGL(cap));
    }
}

void OpenGLRenderStateManager::Disable(Graphics::ServerCapability cap)
{
    if (m_Capabilities[cap])
    {
        m_Capabilities[cap] = false;
        glDisable(GLTranslator::ToGL(cap));
    }
}

void OpenGLRenderStateManager::BlendFunc(Graphics::BlendFactor sfactor, Graphics::BlendFactor dfactor)
{
    if (m_BlendSrc != sfactor || m_BlendDst != dfactor)
    {
        m_BlendSrc = sfactor;
        m_BlendDst = dfactor;
        glBlendFunc(GLTranslator::ToGL(sfactor), GLTranslator::ToGL(dfactor));
    }
}

void OpenGLRenderStateManager::BlendEquation(Graphics::BlendEquation mode)
{
    if (m_BlendEquation != mode)
    {
        m_BlendEquation = mode;
        glBlendEquation(GLTranslator::ToGL(mode));
    }
}

void OpenGLRenderStateManager::DepthFunc(Graphics::CompareFunc func)
{
    if (m_DepthFunc != func)
    {
        m_DepthFunc = func;
        glDepthFunc(GLTranslator::ToGL(func));
    }
}

void OpenGLRenderStateManager::DepthMask(bool flag)
{
    if (m_DepthMask != flag)
    {
        m_DepthMask = flag;
        glDepthMask(flag ? GL_TRUE : GL_FALSE);
    }
}

void OpenGLRenderStateManager::StencilFunc(Graphics::CompareFunc func, int ref, unsigned int mask)
{
    if (m_StencilFunc != func || m_StencilRef != ref || m_StencilMask != mask)
    {
        m_StencilFunc = func;
        m_StencilRef = ref;
        m_StencilMask = mask;
        glStencilFunc(GLTranslator::ToGL(func), ref, mask);
    }
}

void OpenGLRenderStateManager::StencilOp(Graphics::StencilOp sfail, Graphics::StencilOp dpfail, Graphics::StencilOp dppass)
{
    if (m_StencilFail != sfail || m_StencilDepthFail != dpfail || m_StencilPass != dppass)
    {
        m_StencilFail = sfail;
        m_StencilDepthFail = dpfail;
        m_StencilPass = dppass;
        glStencilOp(GLTranslator::ToGL(sfail), GLTranslator::ToGL(dpfail), GLTranslator::ToGL(dppass));
    }
}

void OpenGLRenderStateManager::StencilMask(unsigned int mask)
{
    if (m_StencilWriteMask != mask)
    {
        m_StencilWriteMask = mask;
        glStencilMask(mask);
    }
}

void OpenGLRenderStateManager::CullFace(Graphics::CullMode mode)
{
    if (m_CullMode != mode)
    {
        m_CullMode = mode;
        glCullFace(GLTranslator::ToGL(mode));
    }
}

void OpenGLRenderStateManager::FrontFace(Graphics::FrontFace mode)
{
    if (m_FrontFace != mode)
    {
        m_FrontFace = mode;
        glFrontFace(GLTranslator::ToGL(mode));
    }
}

void OpenGLRenderStateManager::Viewport(int x, int y, int width, int height)
{
    if (m_ViewportX != x || m_ViewportY != y || m_ViewportW != width || m_ViewportH != height)
    {
        m_ViewportX = x; m_ViewportY = y; m_ViewportW = width; m_ViewportH = height;
        glViewport(x, y, width, height);
    }
}

void OpenGLRenderStateManager::Scissor(int x, int y, int width, int height)
{
    if (m_ScissorX != x || m_ScissorY != y || m_ScissorW != width || m_ScissorH != height)
    {
        m_ScissorX = x; m_ScissorY = y; m_ScissorW = width; m_ScissorH = height;
        glScissor(x, y, width, height);
    }
}

void OpenGLRenderStateManager::PolygonMode(Graphics::CullMode face, Graphics::PolygonMode mode)
{
    if (m_PolygonFace != face || m_PolygonMode != mode)
    {
        m_PolygonFace = face;
        m_PolygonMode = mode;
        glPolygonMode(GLTranslator::ToGL(face), GLTranslator::ToGL(mode));
    }
}

Graphics::PolygonMode OpenGLRenderStateManager::GetPolygonMode() const
{
    return m_PolygonMode;
}

void OpenGLRenderStateManager::LineWidth(float width)
{
    if (m_LineWidth != width)
    {
        m_LineWidth = width;
        glLineWidth(width);
    }
}

void OpenGLRenderStateManager::PointSize(float size)
{
    if (m_PointSize != size)
    {
        m_PointSize = size;
        glPointSize(size);
    }
}

void OpenGLRenderStateManager::ColorMask(bool r, bool g, bool b, bool a)
{
    if (m_ColorMaskR != r || m_ColorMaskG != g || m_ColorMaskB != b || m_ColorMaskA != a)
    {
        m_ColorMaskR = r; m_ColorMaskG = g; m_ColorMaskB = b; m_ColorMaskA = a;
        glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE,
                    b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
    }
}

const char *OpenGLRenderStateManager::GetBackendName() const
{
    return "OpenGL";
}
