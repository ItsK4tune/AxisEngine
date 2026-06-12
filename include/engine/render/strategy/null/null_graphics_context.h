#pragma once

#include <render/interface/i_graphics_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_shader_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_query_manager.h>

class NullBufferManager final : public IBufferManager
{
public:
    unsigned int CreateVertexArray() override { return 1; }
    unsigned int GenVertexArray() override { return 1; }
    void BindVertexArray(unsigned int) override {}
    void DeleteVertexArray(unsigned int) override {}
    void DeleteVertexArrays(int, const unsigned int*) override {}
    unsigned int CreateBuffer() override { return 1; }
    unsigned int GenBuffer() override { return 1; }
    void BindBuffer(BufferType, unsigned int) override {}
    void BufferData(BufferType, size_t, const void*, BufferUsage) override {}
    void BufferSubData(BufferType, size_t, size_t, const void*) override {}
    void DeleteBuffer(unsigned int) override {}
    void DeleteBuffers(int, const unsigned int*) override {}
    void BindBufferBase(BufferType, unsigned int, unsigned int) override {}
    void BindBufferRange(BufferType, unsigned int, unsigned int, size_t, size_t) override {}
    void EnableVertexAttribArray(unsigned int) override {}
    void VertexAttribPointer(unsigned int, int, DataType, bool, int, const void*) override {}
    void VertexAttribIPointer(unsigned int, int, DataType, int, const void*) override {}
    void VertexAttribDivisor(unsigned int, unsigned int) override {}
    const char* GetBackendName() const override { return "Null"; }
};

class NullTextureManager final : public ITextureManager
{
public:
    unsigned int CreateTexture() override { return 1; }
    unsigned int GenTexture() override { return 1; }
    void BindTexture(TextureType, unsigned int) override {}
    void DeleteTexture(unsigned int) override {}
    void DeleteTextures(int, const unsigned int*) override {}
    void TexParameteri(TextureType, TextureParameter, int) override {}
    void TexParameterf(TextureType, TextureParameter, float) override {}
    void TexParameterfv(TextureType, TextureParameter, const float*) override {}
    void TexParameteriv(TextureType, TextureParameter, const int*) override {}
    void GenerateMipmap(TextureType) override {}
    void TexImage1D(TextureType, int, InternalFormat, int, int, TextureFormat, DataType, const void*) override {}
    void TexImage2D(TextureType, int, InternalFormat, int, int, int, TextureFormat, DataType, const void*) override {}
    void TexImage3D(TextureType, int, InternalFormat, int, int, int, int, TextureFormat, DataType, const void*) override {}
    void TexSubImage2D(TextureType, int, int, int, int, int, TextureFormat, DataType, const void*) override {}
    void ActiveTexture(TextureUnit) override {}
    void PixelStorei(PixelStoreParam, int) override {}
    const char* GetBackendName() const override { return "Null"; }
};

class NullShaderManager final : public IShaderManager
{
public:
    unsigned int CreateShader(ShaderType) override { return 1; }
    void ShaderSource(unsigned int, const char*) override {}
    void CompileShader(unsigned int) override {}
    void DeleteShader(unsigned int) override {}
    bool GetShaderCompileStatus(unsigned int) override { return true; }
    std::string GetShaderInfoLog(unsigned int) override { return ""; }
    unsigned int CreateProgram() override { return 1; }
    void AttachShader(unsigned int, unsigned int) override {}
    void LinkProgram(unsigned int) override {}
    void UseProgram(unsigned int) override {}
    void DeleteProgram(unsigned int) override {}
    bool GetProgramLinkStatus(unsigned int) override { return true; }
    std::string GetProgramInfoLog(unsigned int) override { return ""; }
    int GetUniformLocation(unsigned int, const char*) override { return 0; }
    void SetUniform1i(int, int) override {}
    void SetUniform1ui(int, unsigned int) override {}
    void SetUniform1f(int, float) override {}
    void SetUniform1fv(int, int, const float*) override {}
    void SetUniform2f(int, float, float) override {}
    void SetUniform2fv(int, const float*) override {}
    void SetUniform3f(int, float, float, float) override {}
    void SetUniform3fv(int, const float*) override {}
    void SetUniform4f(int, float, float, float, float) override {}
    void SetUniform4fv(int, const float*) override {}
    void SetUniformMatrix2fv(int, const float*) override {}
    void SetUniformMatrix3fv(int, const float*) override {}
    void SetUniformMatrix4fv(int, const float*) override {}
    void SetUniform3fvArray(int, int, const float*) override {}
    void SetUniformMatrix4fvArray(int, int, const float*) override {}
    void DispatchCompute(unsigned int, unsigned int, unsigned int) override {}
    void MemoryBarrier(MemoryBarrierBit) override {}
    const char* GetBackendName() const override { return "Null"; }
};

class NullRenderTargetManager final : public IRenderTargetManager
{
public:
    unsigned int CreateFramebuffer() override { return 1; }
    unsigned int GenFramebuffer() override { return 1; }
    void BindFramebuffer(FramebufferTarget, unsigned int) override {}
    void DeleteFramebuffer(unsigned int) override {}
    void DeleteFramebuffers(int, const unsigned int*) override {}
    FramebufferStatus CheckFramebufferStatus(FramebufferTarget) override { return FramebufferStatus::Complete; }

    void FramebufferTexture2D(FramebufferTarget, FramebufferAttachment, TextureType, unsigned int, int) override {}
    void FramebufferTexture(FramebufferTarget, FramebufferAttachment, unsigned int, int) override {}
    void FramebufferTextureLayer(FramebufferTarget, FramebufferAttachment, unsigned int, int, int) override {}

    void BlitFramebuffer(int, int, int, int, int, int, int, int, BufferBit, TextureFilter) override {}

    unsigned int CreateRenderbuffer() override { return 1; }
    void BindRenderbuffer(unsigned int) override {}
    void RenderbufferStorage(InternalFormat, int, int) override {}
    void FramebufferRenderbuffer(FramebufferTarget, FramebufferAttachment, unsigned int) override {}
    void DeleteRenderbuffer(unsigned int) override {}

    void DrawBuffer(FramebufferAttachment) override {}
    void ReadBuffer(FramebufferAttachment) override {}
    void DrawBuffers(int, const FramebufferAttachment*) override {}

    const char* GetBackendName() const override { return "Null"; }
};

class NullRenderStateManager final : public IRenderStateManager
{
public:
    void Enable(ServerCapability) override {}
    void Disable(ServerCapability) override {}
    void SetBlendFunc(BlendFactor, BlendFactor) override {}
    void SetBlendEquation(BlendEquation) override {}
    void SetDepthFunc(CompareFunc) override {}
    void SetDepthMask(bool) override {}
    void SetStencilFunc(CompareFunc, int, unsigned int) override {}
    void SetStencilOp(StencilOp, StencilOp, StencilOp) override {}
    void SetStencilMask(unsigned int) override {}
    void SetCullFace(CullMode) override {}
    void SetFrontFace(FrontFace) override {}
    void SetViewport(int, int, int, int) override {}
    void SetScissor(int, int, int, int) override {}
    void SetPolygonMode(CullMode, PolygonMode) override {}
    PolygonMode GetPolygonMode() const override { return PolygonMode::Fill; }
    void SetLineWidth(float) override {}
    void SetPointSize(float) override {}
    void SetColorMask(bool, bool, bool, bool) override {}
    const char* GetBackendName() const override { return "Null"; }
};

class NullDrawContext final : public IDrawContext
{
public:
    void Clear(BufferBit) override {}
    void ClearColor(float, float, float, float) override {}
    void SetViewport(int, int, int, int) override {}
    void Scissor(int, int, int, int) override {}
    void DrawArrays(Primitive, int, int) override {}
    void DrawElements(Primitive, int, DataType, const void*) override {}
    void DrawArraysInstanced(Primitive, int, int, int) override {}
    void DrawElementsInstanced(Primitive, int, DataType, const void*, int) override {}
    const char* GetBackendName() const override { return "Null"; }
};

class NullQueryManager final : public IQueryManager
{
public:
    uint32_t GenQuery() override { return 1; }
    void DeleteQuery(uint32_t) override {}
    void BeginQuery(QueryType, uint32_t) override {}
    void EndQuery(QueryType) override {}
    bool IsResultAvailable(uint32_t) override { return true; }
    uint32_t GetQueryResult(uint32_t) override { return 0; }
    uint64_t GetQueryResult64(uint32_t queryId) override { return GetQueryResult(queryId); }
};

class NullGraphicsContext final : public IGraphicsContext
{
public:
    NullGraphicsContext() = default;
    ~NullGraphicsContext() override = default;

    bool Initialize() override { return true; }
    void Shutdown() override {}
    void SetViewport(int, int, int, int) override {}
    void SetDepthTest(bool) override {}
    void SetCullFace(bool) override {}
    void SetBlending(bool) override {}
    void SetBlendFunc(BlendFactor, BlendFactor) override {}
    void Clear(BufferBit) override {}

    IBufferManager& GetBufferManager() override { return m_BufferManager; }
    ITextureManager& GetTextureManager() override { return m_TextureManager; }
    IShaderManager& GetShaderManager() override { return m_ShaderManager; }
    IRenderTargetManager& GetRenderTargetManager() override { return m_RenderTargetManager; }
    IRenderStateManager& GetRenderStateManager() override { return m_RenderStateManager; }
    IDrawContext& GetDrawContext() override { return m_DrawContext; }
    IQueryManager& GetQueryManager() override { return m_QueryManager; }

    std::string GetName() const override { return "Null"; }
    bool SupportsLegacyRenderPipeline() const override { return false; }

private:
    NullBufferManager m_BufferManager;
    NullTextureManager m_TextureManager;
    NullShaderManager m_ShaderManager;
    NullRenderTargetManager m_RenderTargetManager;
    NullRenderStateManager m_RenderStateManager;
    NullDrawContext m_DrawContext;
    NullQueryManager m_QueryManager;
};
