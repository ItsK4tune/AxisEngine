#pragma once

#include <rendering/interfaces/i_graphics_context.h>
#include <rendering/interfaces/i_render_target_manager.h>
#include <rendering/interfaces/i_texture_manager.h>
#include <rendering/interfaces/i_buffer_manager.h>
#include <rendering/interfaces/i_draw_context.h>

namespace Graphics {

class GPUFramebuffer {
public:
    GPUFramebuffer(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUFramebuffer() {
        if (m_Handle.IsValid()) {
            m_Context.GetRenderTargetManager().DeleteFramebuffers(1, &m_Handle.id);
        }
    }

    GPUFramebuffer(const GPUFramebuffer&) = delete;
    GPUFramebuffer& operator=(const GPUFramebuffer&) = delete;

    GPUFramebuffer(GPUFramebuffer&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUFramebuffer& operator=(GPUFramebuffer&& other) noexcept {
        if (this != &other) {
            if (m_Handle.IsValid()) {
                m_Context.GetRenderTargetManager().DeleteFramebuffers(1, &m_Handle.id);
            }
            m_Handle = other.m_Handle;
            other.m_Handle.Reset();
        }
        return *this;
    }

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUTexture {
public:
    GPUTexture(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUTexture() {
        if (m_Handle.IsValid()) {
            m_Context.GetTextureManager().DeleteTextures(1, &m_Handle.id);
        }
    }

    GPUTexture(const GPUTexture&) = delete;
    GPUTexture& operator=(const GPUTexture&) = delete;

    GPUTexture(GPUTexture&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUTexture& operator=(GPUTexture&& other) noexcept {
        if (this != &other) {
            if (m_Handle.IsValid()) {
                m_Context.GetTextureManager().DeleteTextures(1, &m_Handle.id);
            }
            m_Handle = other.m_Handle;
            other.m_Handle.Reset();
        }
        return *this;
    }

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUSSBO {
public:
    GPUSSBO(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUSSBO() {
        if (m_Handle.IsValid()) {
            m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
        }
    }

    GPUSSBO(const GPUSSBO&) = delete;
    GPUSSBO& operator=(const GPUSSBO&) = delete;

    GPUSSBO(GPUSSBO&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUSSBO& operator=(GPUSSBO&& other) noexcept {
        if (this != &other) {
            if (m_Handle.IsValid()) {
                m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
            }
            m_Handle = other.m_Handle;
            other.m_Handle.Reset();
        }
        return *this;
    }

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUUBO {
public:
    GPUUBO(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUUBO() {
        if (m_Handle.IsValid()) {
            m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
        }
    }

    GPUUBO(const GPUUBO&) = delete;
    GPUUBO& operator=(const GPUUBO&) = delete;

    GPUUBO(GPUUBO&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUUBO& operator=(GPUUBO&& other) noexcept {
        if (this != &other) {
            if (m_Handle.IsValid()) {
                m_Context.GetBufferManager().DeleteBuffers(1, &m_Handle.id);
            }
            m_Handle = other.m_Handle;
            other.m_Handle.Reset();
        }
        return *this;
    }

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

}
