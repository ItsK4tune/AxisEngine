#pragma once

#include <memory>
#include <interface/graphic/i_graphics_context.h>
#include <graphic/backends/opengl_buffer_manager.h>
#include <graphic/backends/opengl_texture_manager.h>
#include <graphic/backends/opengl_shader_manager.h>
#include <graphic/backends/opengl_render_target_manager.h>
#include <graphic/backends/opengl_render_state_manager.h>
#include <graphic/backends/opengl_draw_context.h>

class OpenGLContext : public IGraphicsContext
{
public:
    bool Init() override;
    void Shutdown() override;

    void SetViewport(int x, int y, int width, int height) override;
    void SetDepthTest(bool enabled) override;
    void SetCullFace(bool enabled) override;

    void Clear(Graphics::BufferBit flags) override;

    IBufferManager &GetBufferManager() override { return m_BufferManager; }
    ITextureManager &GetTextureManager() override { return m_TextureManager; }
    IShaderManager &GetShaderManager() override { return m_ShaderManager; }
    IRenderTargetManager &GetRenderTargetManager() override { return m_RenderTargetManager; }
    IRenderStateManager &GetRenderStateManager() override { return m_RenderStateManager; }
    IDrawContext &GetDrawContext() override { return m_DrawContext; }

    std::string GetName() const override { return "OpenGL"; }

private:
    OpenGLBufferManager m_BufferManager;
    OpenGLTextureManager m_TextureManager;
    OpenGLShaderManager m_ShaderManager;
    OpenGLRenderTargetManager m_RenderTargetManager;
    OpenGLRenderStateManager m_RenderStateManager;
    OpenGLDrawContext m_DrawContext;
};
