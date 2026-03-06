#pragma once

#include <rendering/types/graphics_enums.h>
#include <rendering/types/buffer_types.h>
#include <rendering/types/texture_types.h>
#include <rendering/types/render_state_types.h>
#include <rendering/types/framebuffer_types.h>
#include <rendering/types/graphics_query_types.h>
#include <rendering/types/gpu_handle.h>

class IRenderTargetManager
{
public:
    virtual ~IRenderTargetManager() = default;

    virtual unsigned int CreateFramebuffer() = 0;
    virtual unsigned int GenFramebuffer() = 0;
    virtual void BindFramebuffer(Graphics::FramebufferTarget target, unsigned int fbo) = 0;
    virtual void DeleteFramebuffer(unsigned int fbo) = 0;
    virtual void DeleteFramebuffers(int n, const unsigned int* framebuffers) = 0;
    virtual Graphics::FramebufferStatus CheckFramebufferStatus(Graphics::FramebufferTarget target) = 0;

    virtual void FramebufferTexture2D(Graphics::FramebufferTarget target, Graphics::FramebufferAttachment attachment,
                                      Graphics::TextureType textarget, unsigned int texture, int level) = 0;
    virtual void FramebufferTexture(Graphics::FramebufferTarget target, Graphics::FramebufferAttachment attachment,
                                    unsigned int texture, int level) = 0;

    virtual void BlitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1,
                                 int dstX0, int dstY0, int dstX1, int dstY1,
                                 Graphics::BufferBit mask, Graphics::TextureFilter filter) = 0;

    virtual unsigned int CreateRenderbuffer() = 0;
    virtual void BindRenderbuffer(unsigned int renderbuffer) = 0;
    virtual void RenderbufferStorage(Graphics::InternalFormat internalformat, int width, int height) = 0;
    virtual void FramebufferRenderbuffer(Graphics::FramebufferTarget target, Graphics::FramebufferAttachment attachment,
                                         unsigned int renderbuffer) = 0;
    virtual void DeleteRenderbuffer(unsigned int renderbuffer) = 0;

    virtual void DrawBuffer(Graphics::FramebufferAttachment buf) = 0;
    virtual void ReadBuffer(Graphics::FramebufferAttachment buf) = 0;
    virtual void DrawBuffers(int n, const Graphics::FramebufferAttachment *bufs) = 0;

    virtual const char *GetBackendName() const = 0;
};
