#pragma once

#include <render/type/graphics_types.h>

class IRenderTargetManager
{
public:
    virtual ~IRenderTargetManager() = default;

    virtual unsigned int CreateFramebuffer() = 0;
    virtual unsigned int GenFramebuffer() = 0;
    virtual void BindFramebuffer(FramebufferTarget target, unsigned int fbo) = 0;
    virtual void DeleteFramebuffer(unsigned int fbo) = 0;
    virtual void DeleteFramebuffers(int n, const unsigned int* framebuffers) = 0;
    virtual FramebufferStatus CheckFramebufferStatus(FramebufferTarget target) = 0;

    virtual void FramebufferTexture2D(FramebufferTarget target, FramebufferAttachment attachment, TextureType textarget,
                                      unsigned int Texture, int level) = 0;
    virtual void FramebufferTexture(FramebufferTarget target, FramebufferAttachment attachment, unsigned int Texture,
                                    int level) = 0;
    virtual void FramebufferTextureLayer(FramebufferTarget target, FramebufferAttachment attachment,
                                         unsigned int Texture, int level, int layer) = 0;

    virtual void BlitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1,
                                 BufferBit mask, TextureFilter filter) = 0;

    virtual unsigned int CreateRenderbuffer() = 0;
    virtual void BindRenderbuffer(unsigned int renderbuffer) = 0;
    virtual void RenderbufferStorage(InternalFormat internalformat, int width, int height) = 0;
    virtual void FramebufferRenderbuffer(FramebufferTarget target, FramebufferAttachment attachment,
                                         unsigned int renderbuffer) = 0;
    virtual void DeleteRenderbuffer(unsigned int renderbuffer) = 0;

    virtual void DrawBuffer(FramebufferAttachment buf) = 0;
    virtual void ReadBuffer(FramebufferAttachment buf) = 0;
    virtual void DrawBuffers(int n, const FramebufferAttachment* bufs) = 0;
    virtual void ClearColorAttachmentUInt(int drawBuffer, const unsigned int value[4]) = 0;

    virtual const char* GetBackendName() const = 0;
};
