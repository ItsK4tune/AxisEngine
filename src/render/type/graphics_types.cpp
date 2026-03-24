#include <render/type/graphics_types.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_target_manager.h>



GPUFramebuffer::~GPUFramebuffer() {
    if (m_Handle.IsValid()) {
        m_Context.GetRenderTargetManager().DeleteFramebuffers(1, &m_Handle.id);
    }
}

GPUFramebuffer& GPUFramebuffer::operator=(GPUFramebuffer&& other) noexcept {
    if (this != &other) {
        if (m_Handle.IsValid()) {
            m_Context.GetRenderTargetManager().DeleteFramebuffers(1, &m_Handle.id);
        }
        m_Handle = other.m_Handle;
        other.m_Handle.Reset();
    }
    return *this;
}


GPUSSBO::~GPUSSBO() {
    if (m_Handle.IsValid()) {
        m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
    }
}

GPUSSBO& GPUSSBO::operator=(GPUSSBO&& other) noexcept {
    if (this != &other) {
        if (m_Handle.IsValid()) {
            m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
        }
        m_Handle = other.m_Handle;
        other.m_Handle.Reset();
    }
    return *this;
}


GPUTexture::~GPUTexture() {
    if (m_Handle.IsValid()) {
        m_Context.GetTextureManager().DeleteTextures(1, &m_Handle.id);
    }
}

GPUTexture& GPUTexture::operator=(GPUTexture&& other) noexcept {
    if (this != &other) {
        if (m_Handle.IsValid()) {
            m_Context.GetTextureManager().DeleteTextures(1, &m_Handle.id);
        }
        m_Handle = other.m_Handle;
        other.m_Handle.Reset();
    }
    return *this;
}


GPUUBO::~GPUUBO() {
    if (m_Handle.IsValid()) {
        m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
    }
}

GPUUBO& GPUUBO::operator=(GPUUBO&& other) noexcept {
    if (this != &other) {
        if (m_Handle.IsValid()) {
            m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
        }
        m_Handle = other.m_Handle;
        other.m_Handle.Reset();
    }
    return *this;
}

