#include <render/strategy/opengl/opengl_render_state_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <core/logic/runtime_profiler.h>

void OpenGLRenderStateManager::Enable(ServerCapability cap)
{
    if (cap == ServerCapability::StencilTest)
    {
        auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
        if (cm && !cm->GetConfigSnapshot()->culling.stencilTestEnabled)
        {
            return;
        }
    }

    const size_t capabilityIndex = static_cast<size_t>(cap);
    if (!IsKnown(capabilityIndex) || !m_Capabilities[capabilityIndex])
    {
        m_Capabilities[capabilityIndex] = true;
        glEnable(GLTranslator::ToGL(cap));
        MarkKnown(capabilityIndex);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::Disable(ServerCapability cap)
{
    const size_t capabilityIndex = static_cast<size_t>(cap);
    if (!IsKnown(capabilityIndex) || m_Capabilities[capabilityIndex])
    {
        m_Capabilities[capabilityIndex] = false;
        glDisable(GLTranslator::ToGL(cap));
        MarkKnown(capabilityIndex);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetBlendFunc(BlendFactor sfactor, BlendFactor dfactor)
{
    SetBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void OpenGLRenderStateManager::SetBlendFuncSeparate(BlendFactor srcRgb, BlendFactor dstRgb,
                                                     BlendFactor srcAlpha, BlendFactor dstAlpha)
{
    if (!IsKnown(6) || m_BlendSrc != srcRgb || m_BlendDst != dstRgb || m_BlendSrcAlpha != srcAlpha ||
        m_BlendDstAlpha != dstAlpha)
    {
        m_BlendSrc = srcRgb;
        m_BlendDst = dstRgb;
        m_BlendSrcAlpha = srcAlpha;
        m_BlendDstAlpha = dstAlpha;
        glBlendFuncSeparate(GLTranslator::ToGL(srcRgb), GLTranslator::ToGL(dstRgb),
                            GLTranslator::ToGL(srcAlpha), GLTranslator::ToGL(dstAlpha));
        MarkKnown(6);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetBlendEquation(BlendEquation mode)
{
    if (!IsKnown(7) || m_BlendEquation != mode)
    {
        m_BlendEquation = mode;
        glBlendEquation(GLTranslator::ToGL(mode));
        MarkKnown(7);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetDepthFunc(CompareFunc func)
{
    if (!IsKnown(8) || m_DepthFunc != func)
    {
        m_DepthFunc = func;
        glDepthFunc(GLTranslator::ToGL(func));
        MarkKnown(8);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetDepthMask(bool flag)
{
    if (!IsKnown(9) || m_DepthMask != flag)
    {
        m_DepthMask = flag;
        glDepthMask(flag ? GL_TRUE : GL_FALSE);
        MarkKnown(9);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetStencilFunc(CompareFunc func, int ref, unsigned int mask)
{
    if (!IsKnown(10) || m_StencilFunc != func || m_StencilRef != ref || m_StencilMask != mask)
    {
        m_StencilFunc = func;
        m_StencilRef = ref;
        m_StencilMask = mask;
        glStencilFunc(GLTranslator::ToGL(func), ref, mask);
        MarkKnown(10);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass)
{
    if (!IsKnown(11) || m_StencilFail != sfail || m_StencilDepthFail != dpfail || m_StencilPass != dppass)
    {
        m_StencilFail = sfail;
        m_StencilDepthFail = dpfail;
        m_StencilPass = dppass;
        glStencilOp(GLTranslator::ToGL(sfail), GLTranslator::ToGL(dpfail), GLTranslator::ToGL(dppass));
        MarkKnown(11);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetStencilMask(unsigned int mask)
{
    if (!IsKnown(12) || m_StencilWriteMask != mask)
    {
        m_StencilWriteMask = mask;
        glStencilMask(mask);
        MarkKnown(12);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetCullFace(CullMode mode)
{
    if (!IsKnown(13) || m_CullMode != mode)
    {
        m_CullMode = mode;
        glCullFace(GLTranslator::ToGL(mode));
        MarkKnown(13);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetFrontFace(FrontFace mode)
{
    if (!IsKnown(14) || m_FrontFace != mode)
    {
        m_FrontFace = mode;
        glFrontFace(GLTranslator::ToGL(mode));
        MarkKnown(14);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetViewport(int x, int y, int width, int height)
{
    if (!IsKnown(15) || m_ViewportX != x || m_ViewportY != y || m_ViewportW != width || m_ViewportH != height)
    {
        m_ViewportX = x;
        m_ViewportY = y;
        m_ViewportW = width;
        m_ViewportH = height;
        glViewport(x, y, width, height);
        MarkKnown(15);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetScissor(int x, int y, int width, int height)
{
    if (!IsKnown(16) || m_ScissorX != x || m_ScissorY != y || m_ScissorW != width || m_ScissorH != height)
    {
        m_ScissorX = x;
        m_ScissorY = y;
        m_ScissorW = width;
        m_ScissorH = height;
        glScissor(x, y, width, height);
        MarkKnown(16);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetPolygonMode(CullMode face, PolygonMode mode)
{
    if (!IsKnown(17) || m_PolygonFace != face || m_PolygonMode != mode)
    {
        m_PolygonFace = face;
        m_PolygonMode = mode;
        glPolygonMode(GLTranslator::ToGL(face), GLTranslator::ToGL(mode));
        MarkKnown(17);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

PolygonMode OpenGLRenderStateManager::GetPolygonMode() const
{
    return m_PolygonMode;
}

void OpenGLRenderStateManager::SetLineWidth(float width)
{
    if (!IsKnown(18) || m_LineWidth != width)
    {
        m_LineWidth = width;
        glLineWidth(width);
        MarkKnown(18);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetPointSize(float size)
{
    if (!IsKnown(19) || m_PointSize != size)
    {
        m_PointSize = size;
        glPointSize(size);
        MarkKnown(19);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

void OpenGLRenderStateManager::SetColorMask(bool r, bool g, bool b, bool a)
{
    if (!IsKnown(20) || m_ColorMaskR != r || m_ColorMaskG != g || m_ColorMaskB != b || m_ColorMaskA != a)
    {
        m_ColorMaskR = r;
        m_ColorMaskG = g;
        m_ColorMaskB = b;
        m_ColorMaskA = a;
        glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
        MarkKnown(20);
        RuntimeProfiler::Instance().AddStateChanges();
    }
}

const char* OpenGLRenderStateManager::GetBackendName() const
{
    return "OpenGL";
}
