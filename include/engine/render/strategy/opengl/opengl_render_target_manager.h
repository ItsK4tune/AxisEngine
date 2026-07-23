#pragma once

#include <render/interface/i_render_target_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>
#include <core/logic/runtime_profiler.h>
#include <array>
#include <limits>

class OpenGLRenderTargetManager : public IRenderTargetManager
{
public:
    OpenGLRenderTargetManager()
    {
        InvalidateCache();
    }

    void InvalidateCache()
    {
        m_ReadFramebuffer = InvalidBinding;
        m_DrawFramebuffer = InvalidBinding;
        m_DrawBufferCount = -1;
    }

    void SetCacheEnabled(bool enabled)
    {
        m_CacheEnabled = enabled;
        InvalidateCache();
    }

    unsigned int CreateFramebuffer() override
    {
        unsigned int fbo;
        glGenFramebuffers(1, &fbo);
        return fbo;
    }

    unsigned int GenFramebuffer() override
    {
        return CreateFramebuffer();
    }

    void BindFramebuffer(FramebufferTarget target, unsigned int fbo) override
    {
        unsigned int* cached = target == FramebufferTarget::ReadFramebuffer ? &m_ReadFramebuffer : &m_DrawFramebuffer;
        const bool alreadyBound = target == FramebufferTarget::Framebuffer
                                      ? m_ReadFramebuffer == fbo && m_DrawFramebuffer == fbo
                                      : *cached == fbo;
        if (m_CacheEnabled && alreadyBound)
            return;
        glBindFramebuffer(GLTranslator::ToGL(target), fbo);
        if (target == FramebufferTarget::Framebuffer)
            m_ReadFramebuffer = m_DrawFramebuffer = fbo;
        else
            *cached = fbo;
        if (target != FramebufferTarget::ReadFramebuffer)
            m_DrawBufferCount = -1;
        RuntimeProfiler::Instance().AddStateChanges();
    }
    void DeleteFramebuffer(unsigned int fbo) override
    {
        glDeleteFramebuffers(1, &fbo);
        if (m_ReadFramebuffer == fbo)
            m_ReadFramebuffer = InvalidBinding;
        if (m_DrawFramebuffer == fbo)
            m_DrawFramebuffer = InvalidBinding;
    }
    void DeleteFramebuffers(int n, const unsigned int* framebuffers) override
    {
        glDeleteFramebuffers(n, framebuffers);
        InvalidateCache();
    }
    FramebufferStatus CheckFramebufferStatus(FramebufferTarget target) override
    {
        GLenum status = glCheckFramebufferStatus(GLTranslator::ToGL(target));
        switch (status)
        {
            case GL_FRAMEBUFFER_COMPLETE:
                return FramebufferStatus::Complete;
            case GL_FRAMEBUFFER_UNDEFINED:
                return FramebufferStatus::Undefined;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                return FramebufferStatus::IncompleteAttachment;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                return FramebufferStatus::IncompleteMissingAttachment;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                return FramebufferStatus::IncompleteDrawBuffer;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                return FramebufferStatus::IncompleteReadBuffer;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                return FramebufferStatus::Unsupported;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                return FramebufferStatus::IncompleteMultisample;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                return FramebufferStatus::IncompleteLayerTargets;
            default:
                return FramebufferStatus::Unknown;
        }
    }

    void FramebufferTexture2D(FramebufferTarget target, FramebufferAttachment attachment, TextureType textarget,
                              unsigned int Texture, int level) override
    {
        glFramebufferTexture2D(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment),
                               GLTranslator::ToGL(textarget), Texture, level);
    }

    void BlitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1,
                         BufferBit mask, TextureFilter filter) override
    {
        glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GLTranslator::ToGL(mask),
                          GLTranslator::ToGL(filter));
    }

    void FramebufferTexture(FramebufferTarget target, FramebufferAttachment attachment, unsigned int Texture,
                            int level) override
    {
        glFramebufferTexture(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment), Texture, level);
    }

    void FramebufferTextureLayer(FramebufferTarget target, FramebufferAttachment attachment, unsigned int Texture,
                                 int level, int layer) override
    {
        glFramebufferTextureLayer(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment), Texture, level, layer);
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

    void RenderbufferStorage(InternalFormat internalformat, int width, int height) override
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GLTranslator::ToGL(internalformat), width, height);
    }

    void FramebufferRenderbuffer(FramebufferTarget target, FramebufferAttachment attachment,
                                 unsigned int renderbuffer) override
    {
        glFramebufferRenderbuffer(GLTranslator::ToGL(target), GLTranslator::ToGL(attachment), GL_RENDERBUFFER,
                                  renderbuffer);
    }

    void DeleteRenderbuffer(unsigned int renderbuffer) override
    {
        glDeleteRenderbuffers(1, &renderbuffer);
    }

    void DrawBuffer(FramebufferAttachment buf) override
    {
        glDrawBuffer(GLTranslator::ToGL(buf));
        m_DrawBuffers[0] = buf;
        m_DrawBufferCount = 1;
        RuntimeProfiler::Instance().AddStateChanges();
    }
    void ReadBuffer(FramebufferAttachment buf) override
    {
        glReadBuffer(GLTranslator::ToGL(buf));
    }
    void ReadPixels(int x, int y, int width, int height, TextureFormat format, DataType type, void* data) override
    {
        glReadPixels(x, y, width, height, GLTranslator::ToGL(format), GLTranslator::ToGL(type), data);
    }

    void DrawBuffers(int n, const FramebufferAttachment* bufs) override
    {
        if (n <= 0)
            return;
        if (m_CacheEnabled && n <= static_cast<int>(m_DrawBuffers.size()) && n == m_DrawBufferCount)
        {
            bool unchanged = true;
            for (int i = 0; i < n; ++i)
                unchanged = unchanged && m_DrawBuffers[static_cast<size_t>(i)] == bufs[i];
            if (unchanged)
                return;
        }
        if (n > static_cast<int>(m_DrawBuffers.size()))
            n = static_cast<int>(m_DrawBuffers.size());
        std::array<GLenum, 32> glBufs{};
        for (int i = 0; i < n; ++i)
        {
            glBufs[static_cast<size_t>(i)] = GLTranslator::ToGL(bufs[i]);
            m_DrawBuffers[static_cast<size_t>(i)] = bufs[i];
        }
        glDrawBuffers(n, glBufs.data());
        m_DrawBufferCount = n;
        RuntimeProfiler::Instance().AddStateChanges();
    }

    void ClearColorAttachmentUInt(int drawBuffer, const unsigned int value[4]) override
    {
        if (drawBuffer < 0 || !value)
            return;
        glClearBufferuiv(GL_COLOR, drawBuffer, value);
    }

    const char* GetBackendName() const override
    {
        return "OpenGL";
    }

private:
    static constexpr unsigned int InvalidBinding = (std::numeric_limits<unsigned int>::max)();
    unsigned int m_ReadFramebuffer = InvalidBinding;
    unsigned int m_DrawFramebuffer = InvalidBinding;
    std::array<FramebufferAttachment, 32> m_DrawBuffers = {};
    int m_DrawBufferCount = -1;
    bool m_CacheEnabled = true;
};
