#include <render/strategy/opengl/opengl_render_state_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>

void OpenGLRenderStateManager::Enable(ServerCapability cap)
{
    if (cap == ServerCapability::StencilTest)
    {
        auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
        if (cm && !cm->GetConfig().culling.stencilTestEnabled)
        {
            return;
        }
    }

    if (!m_Capabilities[cap])
    {
        m_Capabilities[cap] = true;
        glEnable(GLTranslator::ToGL(cap));
    }
}

void OpenGLRenderStateManager::Disable(ServerCapability cap)
{
    if (m_Capabilities[cap])
    {
        m_Capabilities[cap] = false;
        glDisable(GLTranslator::ToGL(cap));
    }
}

void OpenGLRenderStateManager::SetBlendFunc(BlendFactor sfactor, BlendFactor dfactor)
{
    if (m_BlendSrc != sfactor || m_BlendDst != dfactor)
    {
        m_BlendSrc = sfactor;
        m_BlendDst = dfactor;
        glBlendFunc(GLTranslator::ToGL(sfactor), GLTranslator::ToGL(dfactor));
    }
}

void OpenGLRenderStateManager::SetBlendEquation(BlendEquation mode)
{
    if (m_BlendEquation != mode)
    {
        m_BlendEquation = mode;
        glBlendEquation(GLTranslator::ToGL(mode));
    }
}

void OpenGLRenderStateManager::SetDepthFunc(CompareFunc func)
{
    if (m_DepthFunc != func)
    {
        m_DepthFunc = func;
        glDepthFunc(GLTranslator::ToGL(func));
    }
}

void OpenGLRenderStateManager::SetDepthMask(bool flag)
{
    if (m_DepthMask != flag)
    {
        m_DepthMask = flag;
        glDepthMask(flag ? GL_TRUE : GL_FALSE);
    }
}

void OpenGLRenderStateManager::SetStencilFunc(CompareFunc func, int ref, unsigned int mask)
{
    if (m_StencilFunc != func || m_StencilRef != ref || m_StencilMask != mask)
    {
        m_StencilFunc = func;
        m_StencilRef = ref;
        m_StencilMask = mask;
        glStencilFunc(GLTranslator::ToGL(func), ref, mask);
    }
}

void OpenGLRenderStateManager::SetStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass)
{
    if (m_StencilFail != sfail || m_StencilDepthFail != dpfail || m_StencilPass != dppass)
    {
        m_StencilFail = sfail;
        m_StencilDepthFail = dpfail;
        m_StencilPass = dppass;
        glStencilOp(GLTranslator::ToGL(sfail), GLTranslator::ToGL(dpfail), GLTranslator::ToGL(dppass));
    }
}

void OpenGLRenderStateManager::SetStencilMask(unsigned int mask)
{
    if (m_StencilWriteMask != mask)
    {
        m_StencilWriteMask = mask;
        glStencilMask(mask);
    }
}

void OpenGLRenderStateManager::SetCullFace(CullMode mode)
{
    if (m_CullMode != mode)
    {
        m_CullMode = mode;
        glCullFace(GLTranslator::ToGL(mode));
    }
}

void OpenGLRenderStateManager::SetFrontFace(FrontFace mode)
{
    if (m_FrontFace != mode)
    {
        m_FrontFace = mode;
        glFrontFace(GLTranslator::ToGL(mode));
    }
}

void OpenGLRenderStateManager::SetViewport(int x, int y, int width, int height)
{
    if (m_ViewportX != x || m_ViewportY != y || m_ViewportW != width || m_ViewportH != height)
    {
        m_ViewportX = x;
        m_ViewportY = y;
        m_ViewportW = width;
        m_ViewportH = height;
        glViewport(x, y, width, height);
    }
}

void OpenGLRenderStateManager::SetScissor(int x, int y, int width, int height)
{
    if (m_ScissorX != x || m_ScissorY != y || m_ScissorW != width || m_ScissorH != height)
    {
        m_ScissorX = x;
        m_ScissorY = y;
        m_ScissorW = width;
        m_ScissorH = height;
        glScissor(x, y, width, height);
    }
}

void OpenGLRenderStateManager::SetPolygonMode(CullMode face, PolygonMode mode)
{
    if (m_PolygonFace != face || m_PolygonMode != mode)
    {
        m_PolygonFace = face;
        m_PolygonMode = mode;
        glPolygonMode(GLTranslator::ToGL(face), GLTranslator::ToGL(mode));
    }
}

PolygonMode OpenGLRenderStateManager::GetPolygonMode() const
{
    return m_PolygonMode;
}

void OpenGLRenderStateManager::SetLineWidth(float width)
{
    if (m_LineWidth != width)
    {
        m_LineWidth = width;
        glLineWidth(width);
    }
}

void OpenGLRenderStateManager::SetPointSize(float size)
{
    if (m_PointSize != size)
    {
        m_PointSize = size;
        glPointSize(size);
    }
}

void OpenGLRenderStateManager::SetColorMask(bool r, bool g, bool b, bool a)
{
    if (m_ColorMaskR != r || m_ColorMaskG != g || m_ColorMaskB != b || m_ColorMaskA != a)
    {
        m_ColorMaskR = r;
        m_ColorMaskG = g;
        m_ColorMaskB = b;
        m_ColorMaskA = a;
        glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
    }
}

const char* OpenGLRenderStateManager::GetBackendName() const
{
    return "OpenGL";
}
