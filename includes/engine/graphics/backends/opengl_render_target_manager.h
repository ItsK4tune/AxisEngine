#pragma once

#include <glad/glad.h>
#include <graphics/backends/opengl_translator.h>
#include <graphics/interfaces/i_render_target_manager.h>
#include <vector>

class OpenGLRenderTargetManager : public IRenderTargetManager
{
public:
    unsigned int CreateFramebuffer() override
    {
        unsigned int fbo;
        glGenFramebuffers(1, &fbo);
        return fbo;
    }

    unsigned int GenFramebuffer() override { return CreateFramebuffer(); }

    void BindFramebuffer(Graphics::FramebufferTarget target, unsigned int fbo) override { glBindFramebuffer(GLTranslator::ToGL(target), fbo); }
    void DeleteFramebuffer(unsigned int fbo) override { glDeleteFramebuffers(1, &fbo); }
    void DeleteFramebuffers(int n, const unsigned int* framebuffers) override { glDeleteFramebuffers(n, framebuffers); }
    Graphics::FramebufferStatus CheckFramebufferStatus(Graphics::FramebufferTarget target) override
    {
        GLenum status = glCheckFramebufferStatus(GLTranslator::ToGL(target));
        switch(status) {
            case GL_FRAMEBUFFER_COMPLETE: return Graphics::FramebufferStatus::Complete;
            case GL_FRAMEBUFFER_UNDEFINED: return Graphics::FramebufferStatus::Undefined;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return Graphics::FramebufferStatus::IncompleteAttachment;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return Graphics::FramebufferStatus::IncompleteMissingAttachment;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return Graphics::FramebufferStatus::IncompleteDrawBuffer;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return Graphics::FramebufferStatus::IncompleteReadBuffer;
            case GL_FRAMEBUFFER_UNSUPPORTED: return Graphics::FramebufferStatus::Unsupported;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return Graphics::FramebufferStatus::IncompleteMultisample;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return Graphics::FramebufferStatus::IncompleteLayerTargets;
            default: return Graphics::FramebufferStatus::Unknown;
        }
    }

    void FramebufferTexture2D(Graphics::FramebufferTarget target, Graphics::FramebufferAttachment attachment,
                              Graphics::TextureType textarget, unsigned int texture, int level) override
    {
        glFramebufferTexture2D(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment), GLTranslator::ToGL(textarget), texture, level);
    }

    void BlitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1,
                         int dstX0, int dstY0, int dstX1, int dstY1,
                         Graphics::BufferBit mask, Graphics::TextureFilter filter) override
    {
        glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GLTranslator::ToGL(mask), GLTranslator::ToGL(filter));
    }

    void FramebufferTexture(Graphics::FramebufferTarget target, Graphics::FramebufferAttachment attachment,
                            unsigned int texture, int level) override
    {
        glFramebufferTexture(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment), texture, level);
    }

    unsigned int CreateRenderbuffer() override
    {
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        return rbo;
    }

    void BindRenderbuffer(unsigned int renderbuffer) override
    {
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    }

    void RenderbufferStorage(Graphics::InternalFormat internalformat,
                             int width, int height) override
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GLTranslator::ToGL(internalformat), width, height);
    }

    void FramebufferRenderbuffer(Graphics::FramebufferTarget target, Graphics::FramebufferAttachment attachment,
                                 unsigned int renderbuffer) override
    {
        glFramebufferRenderbuffer(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment), GL_RENDERBUFFER, renderbuffer);
    }

    void DeleteRenderbuffer(unsigned int renderbuffer) override { glDeleteRenderbuffers(1, &renderbuffer); }

    void DrawBuffer(Graphics::FramebufferAttachment buf) override { glDrawBuffer(GLTranslator::ToGL(buf)); }
    void ReadBuffer(Graphics::FramebufferAttachment buf) override { glReadBuffer(GLTranslator::ToGL(buf)); }

    void DrawBuffers(int n, const Graphics::FramebufferAttachment *bufs) override
    {
        if (n <= 0) return;
        std::vector<GLenum> glBufs(n);
        for(int i = 0; i < n; ++i) {
            glBufs[i] = GLTranslator::ToGL(bufs[i]);
        }
        glDrawBuffers(n, glBufs.data());
    }

    const char *GetBackendName() const override { return "OpenGL"; }
};
