#pragma once

#include <rendering/backends/opengl_buffer_manager.h>
#include <rendering/backends/opengl_draw_context.h>
#include <rendering/backends/opengl_query_manager.h>
#include <rendering/backends/opengl_render_state_manager.h>
#include <rendering/backends/opengl_render_target_manager.h>
#include <rendering/backends/opengl_shader_manager.h>
#include <rendering/backends/opengl_texture_manager.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <memory>

class OpenGLContext : public IGraphicsContext
{
public:
    bool Init() override;
    void Shutdown() override;

    void SetViewport(int x, int y, int width, int height) override;
    void SetDepthTest(bool enabled) override;
    void SetCullFace(bool enabled) override;
    void SetBlending(bool enabled) override;
    void SetBlendFunc(Graphics::BlendFactor src, Graphics::BlendFactor dst) override;

    void Clear(Graphics::BufferBit flags) override;

    IBufferManager &GetBufferManager() override { return m_BufferManager; }
    ITextureManager &GetTextureManager() override { return m_TextureManager; }
    IShaderManager &GetShaderManager() override { return m_ShaderManager; }
    IRenderTargetManager &GetRenderTargetManager() override { return m_RenderTargetManager; }
    IRenderStateManager &GetRenderStateManager() override { return m_RenderStateManager; }
    IDrawContext &GetDrawContext() override { return m_DrawContext; }
    IQueryManager &GetQueryManager() override { return m_QueryManager; }

    std::string GetName() const override { return "OpenGL"; }

private:
    OpenGLBufferManager m_BufferManager;
    OpenGLTextureManager m_TextureManager;
    OpenGLShaderManager m_ShaderManager;
    OpenGLRenderTargetManager m_RenderTargetManager;
    OpenGLRenderStateManager m_RenderStateManager;
    OpenGLDrawContext m_DrawContext;
    OpenGLQueryManager m_QueryManager;
};
