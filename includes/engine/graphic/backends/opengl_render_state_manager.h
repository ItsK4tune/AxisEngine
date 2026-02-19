#pragma once

#include <interface/graphic/i_render_state_manager.h>
#include <graphic/backends/opengl_translator.h>
#include <glad/glad.h>

class OpenGLRenderStateManager : public IRenderStateManager
{
public:
    void Enable(Graphics::ServerCapability cap) override { glEnable(GLTranslator::ToGL(cap)); }
    void Disable(Graphics::ServerCapability cap) override { glDisable(GLTranslator::ToGL(cap)); }

    void BlendFunc(Graphics::BlendFactor sfactor, Graphics::BlendFactor dfactor) override { 
        glBlendFunc(GLTranslator::ToGL(sfactor), GLTranslator::ToGL(dfactor)); 
    }
    void BlendEquation(Graphics::BlendEquation mode) override { glBlendEquation(GLTranslator::ToGL(mode)); }

    void DepthFunc(Graphics::CompareFunc func) override { glDepthFunc(GLTranslator::ToGL(func)); }
    void DepthMask(bool flag) override { glDepthMask(flag ? GL_TRUE : GL_FALSE); }

    void StencilFunc(Graphics::CompareFunc func, int ref, unsigned int mask) override { 
        glStencilFunc(GLTranslator::ToGL(func), ref, mask); 
    }
    void StencilOp(Graphics::StencilOp sfail, Graphics::StencilOp dpfail, Graphics::StencilOp dppass) override { 
        glStencilOp(GLTranslator::ToGL(sfail), GLTranslator::ToGL(dpfail), GLTranslator::ToGL(dppass)); 
    }
    void StencilMask(unsigned int mask) override { glStencilMask(mask); }

    void CullFace(Graphics::CullMode mode) override { glCullFace(GLTranslator::ToGL(mode)); }
    void FrontFace(Graphics::FrontFace mode) override { glFrontFace(GLTranslator::ToGL(mode)); }

    void Viewport(int x, int y, int width, int height) override { glViewport(x, y, width, height); }
    void Scissor(int x, int y, int width, int height) override { glScissor(x, y, width, height); }

    void PolygonMode(Graphics::CullMode face, Graphics::PolygonMode mode) override
    {
        glPolygonMode(GLTranslator::ToGL(face), GLTranslator::ToGL(mode));
    }

    Graphics::PolygonMode GetPolygonMode() const override
    {
        GLint mode;
        glGetIntegerv(GL_POLYGON_MODE, &mode);
        // Assuming GL_FRONT_AND_BACK or just getting the value
        if (mode == GL_POINT) return Graphics::PolygonMode::Point;
        if (mode == GL_LINE) return Graphics::PolygonMode::Line;
        return Graphics::PolygonMode::Fill;
    }
    void LineWidth(float width) override { glLineWidth(width); }
    void PointSize(float size) override { glPointSize(size); }

    void ColorMask(bool r, bool g, bool b, bool a) override
    {
        glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE,
                    b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);
    }

    const char *GetBackendName() const override { return "OpenGL"; }
};
