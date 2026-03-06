#include <ecs/systems/render_system.h>
#include <rendering/core/post_process_pipeline.h>
#include <rendering/interfaces/i_buffer_manager.h>
#include <rendering/interfaces/i_draw_context.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <rendering/interfaces/i_render_state_manager.h>
#include <rendering/interfaces/i_render_target_manager.h>
#include <rendering/interfaces/i_texture_manager.h>
#include <resource/resource_manager.h>
#include <core/utils/logger.h>

PostProcessPipeline::PostProcessPipeline() {}

PostProcessPipeline::~PostProcessPipeline()
{
    Shutdown();
}

void PostProcessPipeline::Init(IGraphicsContext& context, int width, int height, IShaderLibrary &shaderLib)
{
    m_Context = &context;
    m_Width = width;
    m_Height = height;
    InitQuad();
    InitFramebuffers();

    shaderLib.LoadShader("fxaa", "includes/engine/asset/shaders/fxaa.vs", "includes/engine/asset/shaders/fxaa.fs");
    shaderLib.LoadShader("taa", "includes/engine/asset/shaders/taa.vs", "includes/engine/asset/shaders/taa.fs");

    m_FXAAShader = shaderLib.GetShader("fxaa");
    m_TAAShader = shaderLib.GetShader("taa");
}

void PostProcessPipeline::InitFramebuffers()
{
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();

    m_PingPong.fbo[0] = std::make_unique<Graphics::GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
    m_PingPong.fbo[1] = std::make_unique<Graphics::GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
    m_PingPong.color[0] = std::make_unique<Graphics::GPUTexture>(*m_Context, tm.GenTexture());
    m_PingPong.color[1] = std::make_unique<Graphics::GPUTexture>(*m_Context, tm.GenTexture());
    m_DepthTexture = std::make_unique<Graphics::GPUTexture>(*m_Context, tm.GenTexture());

    tm.BindTexture(Graphics::TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::DepthComponent24, m_Width, m_Height, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToEdge));

    for (unsigned int i = 0; i < 2; i++)
    {
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_PingPong.fbo[i]->Get());
        tm.BindTexture(Graphics::TextureType::Texture2D, m_PingPong.color[i]->Get());

        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA16F, m_Width, m_Height, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::Float, NULL);
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Linear));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Linear));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToEdge));

        rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Color0, Graphics::TextureType::Texture2D, m_PingPong.color[i]->Get(), 0);

        if (i == 0)
        {
            rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, Graphics::TextureType::Texture2D, m_DepthTexture->Get(), 0);
        }

        if (rtm.CheckFramebufferStatus(Graphics::FramebufferTarget::Framebuffer) != Graphics::FramebufferStatus::Complete)
            LOGGER_ERROR("PostProcess") << "FBO " << i << " is not complete!";
    }

    m_HistoryFBO = std::make_unique<Graphics::GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
    m_HistoryTexture = std::make_unique<Graphics::GPUTexture>(*m_Context, tm.GenTexture());

    rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_HistoryFBO->Get());
    tm.BindTexture(Graphics::TextureType::Texture2D, m_HistoryTexture->Get());
    tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA16F, m_Width, m_Height, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::Float, NULL);
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Linear));
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Linear));
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
    rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Color0, Graphics::TextureType::Texture2D, m_HistoryTexture->Get(), 0);

    if (rtm.CheckFramebufferStatus(Graphics::FramebufferTarget::Framebuffer) != Graphics::FramebufferStatus::Complete)
        LOGGER_ERROR("PostProcess") << "History FBO is not complete!";

    rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::Shutdown()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();

    m_PingPong.fbo[0].reset(); m_PingPong.fbo[1].reset();
    m_PingPong.color[0].reset(); m_PingPong.color[1].reset();
    m_DepthTexture.reset();
    m_HistoryFBO.reset();
    m_HistoryTexture.reset();
    if (m_QuadVAO.IsValid()) { bm.DeleteVertexArrays(1, &m_QuadVAO.id); m_QuadVAO.Reset(); }
    if (m_QuadVBO.IsValid()) { bm.DeleteBuffers(1, &m_QuadVBO.id); m_QuadVBO.Reset(); }
}

void PostProcessPipeline::Resize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    if (!m_Context) return;
    auto& tm = m_Context->GetTextureManager();

    for (unsigned int i = 0; i < 2; i++)
    {
        tm.BindTexture(Graphics::TextureType::Texture2D, m_PingPong.color[i]->Get());
        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA16F, width, height, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::Float, NULL);
    }

    tm.BindTexture(Graphics::TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::DepthComponent24, width, height, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);

    tm.BindTexture(Graphics::TextureType::Texture2D, m_HistoryTexture->Get());
    tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA16F, width, height, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::Float, NULL);
}

void PostProcessPipeline::BeginCapture()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& dc = m_Context->GetDrawContext();

    rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_PingPong.fbo[0]->Get());
    rsm.Enable(Graphics::ServerCapability::DepthTest);
    dc.ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    dc.Clear(Graphics::BufferBit::Color | Graphics::BufferBit::Depth);
}

void PostProcessPipeline::EndCapture()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    rsm.Disable(Graphics::ServerCapability::DepthTest);

    int readIdx = 0;
    int writeIdx = 1;

    for (const auto &effect : m_Effects)
    {
        if (!effect.shader)
            continue;

        rtm.BindFramebuffer(Graphics::FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[readIdx]->Get());
        rtm.BindFramebuffer(Graphics::FramebufferTarget::DrawFramebuffer, m_PingPong.fbo[writeIdx]->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, Graphics::BufferBit::Color, Graphics::TextureFilter::Nearest);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_PingPong.fbo[writeIdx]->Get());

        effect.shader->use();
        effect.shader->setInt("screenTexture", 0);

        tm.ActiveTexture(Graphics::TextureUnit::Texture0);
        tm.BindTexture(Graphics::TextureType::Texture2D, m_PingPong.color[readIdx]->Get());

        bm.BindVertexArray(m_QuadVAO.id);

        if (effect.width > 0 && effect.height > 0)
        {
            rsm.Enable(Graphics::ServerCapability::ScissorTest);
            dc.Scissor(effect.x, m_Height - effect.y - effect.height, effect.width, effect.height);
        }

        dc.DrawArrays(Graphics::Primitive::Triangles, 0, 6);

        if (effect.width > 0 && effect.height > 0)
        {
            rsm.Disable(Graphics::ServerCapability::ScissorTest);
        }

        readIdx = writeIdx;
        writeIdx = 1 - readIdx;
    }

    rtm.BindFramebuffer(Graphics::FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[readIdx]->Get());
    rtm.BindFramebuffer(Graphics::FramebufferTarget::DrawFramebuffer, 0);

    dc.ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    dc.Clear(Graphics::BufferBit::Color);

    rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, Graphics::BufferBit::Color, Graphics::TextureFilter::Nearest);

    bm.BindVertexArray(0);
    rsm.Enable(Graphics::ServerCapability::DepthTest);
}

void PostProcessPipeline::ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4 &prevViewProj, const glm::mat4 &currViewProj, const glm::vec2 &jitterOffset)
{
    if (mode == AntiAliasingMode::NONE || !m_Context)
        return;

    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    std::shared_ptr<Shader> shader;
    if (mode == AntiAliasingMode::FXAA)
        shader = m_FXAAShader;
    else if (mode == AntiAliasingMode::TAA)
        shader = m_TAAShader;

    if (!shader)
    {
        LOGGER_ERROR("PostProcess") << "AA Shader not found for mode " << (int)mode << "!";
        return;
    }

    rsm.Disable(Graphics::ServerCapability::DepthTest);

    shader->use();
    tm.ActiveTexture(Graphics::TextureUnit::Texture0);
    tm.BindTexture(Graphics::TextureType::Texture2D, m_PingPong.color[0]->Get());
    shader->setInt("screenTexture", 0);

    if (mode == AntiAliasingMode::TAA)
    {
        shader->setInt("depthTexture", 1);
        tm.ActiveTexture(Graphics::TextureUnit::Texture1);
        tm.BindTexture(Graphics::TextureType::Texture2D, m_DepthTexture->Get());

        shader->setInt("historyTexture", 2);
        tm.ActiveTexture(Graphics::TextureUnit::Texture2);
        tm.BindTexture(Graphics::TextureType::Texture2D, m_HistoryTexture->Get());

        shader->setMat4("invViewProj", glm::inverse(currViewProj));
        shader->setMat4("prevViewProj", prevViewProj);
        shader->setVec2("jitterOffset", jitterOffset);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        dc.Clear(Graphics::BufferBit::Color);

        bm.BindVertexArray(m_QuadVAO.id);
        dc.DrawArrays(Graphics::Primitive::Triangles, 0, 6);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[1]->Get());
        rtm.BindFramebuffer(Graphics::FramebufferTarget::DrawFramebuffer, m_HistoryFBO->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, Graphics::BufferBit::Color, Graphics::TextureFilter::Nearest);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[1]->Get());
        rtm.BindFramebuffer(Graphics::FramebufferTarget::DrawFramebuffer, m_PingPong.fbo[0]->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, Graphics::BufferBit::Color, Graphics::TextureFilter::Nearest);
    }
    else if (mode == AntiAliasingMode::FXAA)
    {
        shader->setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        dc.Clear(Graphics::BufferBit::Color);

        bm.BindVertexArray(m_QuadVAO.id);
        dc.DrawArrays(Graphics::Primitive::Triangles, 0, 6);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[1]->Get());
        rtm.BindFramebuffer(Graphics::FramebufferTarget::DrawFramebuffer, m_PingPong.fbo[0]->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, Graphics::BufferBit::Color, Graphics::TextureFilter::Nearest);
    }

    bm.BindVertexArray(0);
    rsm.Enable(Graphics::ServerCapability::DepthTest);
    rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader)
{
    if (shader)
    {
        m_Effects.push_back({shader, 0, 0, 0, 0});
    }
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h)
{
    if (shader)
    {
        m_Effects.push_back({shader, x, y, w, h});
    }
}

void PostProcessPipeline::ClearEffects()
{
    m_Effects.clear();
}

void PostProcessPipeline::InitQuad()
{
    if (!m_Context) return;
    auto& bm = m_Context->GetBufferManager();

    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};

    m_QuadVAO.id = bm.GenVertexArray();
    m_QuadVBO.id = bm.GenBuffer();

    bm.BindVertexArray(m_QuadVAO.id);
    bm.BindBuffer(Graphics::BufferType::ArrayBuffer, m_QuadVBO.id);
    bm.BufferData(Graphics::BufferType::ArrayBuffer, sizeof(quadVertices), &quadVertices, Graphics::BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 2, Graphics::DataType::Float, false, 4 * sizeof(float), (void *)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 2, Graphics::DataType::Float, false, 4 * sizeof(float), (void *)(2 * sizeof(float)));
}
