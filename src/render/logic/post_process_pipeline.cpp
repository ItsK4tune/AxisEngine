#include <render/logic/post_process_pipeline.h>
#include <ecs/logic/render_system.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/logger.h>

PostProcessPipeline::PostProcessPipeline() {}

PostProcessPipeline::~PostProcessPipeline()
{
    Shutdown();
}

void PostProcessPipeline::Initialize(IGraphicsContext& context, int width, int height, IShaderLibrary &shaderLib)
{
    m_Context = &context;
    m_Width = width;
    m_Height = height;

    InitQuad();
    InitFramebuffers();


    m_FXAAShader = shaderLib.GetShader("fxaa");
    m_TAAShader = shaderLib.GetShader("taa");
    m_BloomDownsampleShader = shaderLib.GetShader("bloom_down");
    m_BloomUpsampleShader = shaderLib.GetShader("bloom_up");
    m_HDRFinalShader = shaderLib.GetShader("hdr_final");

    UpdateConfig();
}

void PostProcessPipeline::InitFramebuffers()
{
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();

    m_PingPong.fbo[0] = std::make_unique<GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
    m_PingPong.fbo[1] = std::make_unique<GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
    m_PingPong.color[0] = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    m_PingPong.color[1] = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    m_DepthTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());

    tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, m_Width, m_Height, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));

    for (unsigned int i = 0; i < 2; i++)
    {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[i]->Get());
        tm.BindTexture(TextureType::Texture2D, m_PingPong.color[i]->Get());

        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, m_Width, m_Height, 0, TextureFormat::RGBA, DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));

        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D, m_PingPong.color[i]->Get(), 0);

        if (i == 0)
        {
            rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D, m_DepthTexture->Get(), 0);
        }

        if (rtm.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
            LOGGER_ERROR("PostProcess") << "FBO " << i << " is not complete!";
    }

    m_HistoryFBO = std::make_unique<GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
    m_HistoryTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_HistoryFBO->Get());
    tm.BindTexture(TextureType::Texture2D, m_HistoryTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, m_Width, m_Height, 0, TextureFormat::RGBA, DataType::Float, NULL);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D, m_HistoryTexture->Get(), 0);

    if (rtm.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
        LOGGER_ERROR("PostProcess") << "History FBO is not complete!";


    m_BloomMips.clear();
    int bloomW = m_Width / 2;
    int bloomH = m_Height / 2;
    for (int i = 0; i < BLOOM_MIP_COUNT; i++) {
        BloomMip mip;
        mip.width = bloomW;
        mip.height = bloomH;
        mip.texture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
        tm.BindTexture(TextureType::Texture2D, mip.texture->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, bloomW, bloomH, 0, TextureFormat::RGBA, DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));

        m_BloomMips.push_back(std::move(mip));
        bloomW /= 2;
        bloomH /= 2;
        if (bloomW <= 0 || bloomH <= 0) break;
    }

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::UpdateConfig()
{
    auto& cfg = ServiceLocator::Instance().Require<ConfigManager>().GetConfig();
    m_ClearColor = glm::vec4(cfg.clearColor[0], cfg.clearColor[1], cfg.clearColor[2], cfg.clearColor[3]);
    m_BloomEnabled = cfg.bloomEnabled;
    m_BloomThreshold = cfg.bloomThreshold;
    m_BloomIntensity = cfg.bloomIntensity;
    m_BloomRadius = cfg.bloomRadius;
    m_Exposure = cfg.exposure;
    m_Gamma = cfg.gamma;
    m_TonemappingMode = (int)cfg.tonemappingMode;
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
    m_BloomMips.clear();
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
        tm.BindTexture(TextureType::Texture2D, m_PingPong.color[i]->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, width, height, 0, TextureFormat::RGBA, DataType::Float, NULL);
    }

    tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, width, height, 0, TextureFormat::DepthComponent, DataType::Float, NULL);

    tm.BindTexture(TextureType::Texture2D, m_HistoryTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, width, height, 0, TextureFormat::RGBA, DataType::Float, NULL);

    int bloomW = width / 2;
    int bloomH = height / 2;
    for (auto& mip : m_BloomMips) {
        mip.width = bloomW;
        mip.height = bloomH;
        tm.BindTexture(TextureType::Texture2D, mip.texture->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, bloomW, bloomH, 0, TextureFormat::RGBA, DataType::Float, NULL);
        bloomW /= 2;
        bloomH /= 2;
        if (bloomW <= 0 || bloomH <= 0) bloomW = bloomH = 1;
    }
}

void PostProcessPipeline::BeginCapture()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& dc = m_Context->GetDrawContext();

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[0]->Get());
    rsm.SetViewport(0, 0, m_Width, m_Height);
    rsm.Enable(ServerCapability::DepthTest);
    dc.ClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    dc.Clear(BufferBit::Color | BufferBit::Depth);
}

void PostProcessPipeline::EndCapture()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    rsm.Disable(ServerCapability::DepthTest);

    // 1. Pre-Bloom Effects (Priority < 0)
    RenderEffectsRange(-9999, -1);

    // 2. Engine Bloom
    if (m_BloomEnabled) {
        RenderBloom(m_PingPong.CurrentColor().Get());
    }

    // 3. Post-Bloom / Pre-HDR Effects (Priority 0 - 99)
    RenderEffectsRange(0, 99);

    // 4. Engine HDR / Tonemapping
    bool hasLateEffects = false;
    for (const auto& eff : m_Effects) {
        if (eff.priority >= 100) {
            hasLateEffects = true;
            break;
        }
    }

    if (hasLateEffects) {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.PreviousFBO().Get());
    } else {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
        rsm.SetViewport(0, 0, m_Width, m_Height);
    }
    
    dc.Clear(BufferBit::Color);

    m_HDRFinalShader->use();
    m_HDRFinalShader->setInt("screenTexture", 0);
    m_HDRFinalShader->setInt("bloomBlur", 1);
    m_HDRFinalShader->setFloat("exposure", m_Exposure);
    m_HDRFinalShader->setFloat("bloomIntensity", m_BloomIntensity);
    m_HDRFinalShader->setFloat("gamma", m_Gamma);
    m_HDRFinalShader->setInt("tonemappingMode", m_TonemappingMode);

    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, m_PingPong.CurrentColor().Get());

    tm.ActiveTexture(TextureUnit::Texture1);
    if (m_BloomEnabled && !m_BloomMips.empty()) {
        tm.BindTexture(TextureType::Texture2D, m_BloomMips[0].texture->Get());
    } else {
        // Just bind some dummy if bloom is disabled
        tm.BindTexture(TextureType::Texture2D, m_PingPong.PreviousColor().Get());
    }

    bm.BindVertexArray(m_QuadVAO.id);
    dc.DrawArrays(Primitive::Triangles, 0, 6);

    if (hasLateEffects) {
        m_PingPong.Swap();
        // 5. Post-HDR Effects (Priority >= 100)
        RenderEffectsRange(100, 9999);

        // Final blit to screen
        rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.CurrentFBO().Get());
        rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, 0);
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color, TextureFilter::Nearest);
    }

    bm.BindVertexArray(0);
    rsm.Enable(ServerCapability::DepthTest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::RenderEffectsRange(int minPriority, int maxPriority)
{
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    for (const auto &effect : m_Effects)
    {
        if (!effect.shader || effect.priority < minPriority || effect.priority > maxPriority)
            continue;

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.PreviousFBO().Get());

        effect.shader->use();
        effect.shader->setInt("screenTexture", 0);

        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, m_PingPong.CurrentColor().Get());

        bm.BindVertexArray(m_QuadVAO.id);

        if (effect.width > 0 && effect.height > 0)
        {
            rsm.Enable(ServerCapability::ScissorTest);
            dc.Scissor(effect.x, m_Height - effect.y - effect.height, effect.width, effect.height);
        }

        dc.DrawArrays(Primitive::Triangles, 0, 6);

        if (effect.width > 0 && effect.height > 0)
        {
            rsm.Disable(ServerCapability::ScissorTest);
        }

        m_PingPong.Swap();
    }
}

void PostProcessPipeline::RenderBloom(uint32_t srcTexture)
{
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    bm.BindVertexArray(m_QuadVAO.id);

    m_BloomDownsampleShader->use();
    uint32_t currentSrc = srcTexture;
    int currentW = m_Width;
    int currentH = m_Height;

    for (const auto& mip : m_BloomMips) {
        m_BloomDownsampleShader->setVec2("srcResolution", glm::vec2(currentW, currentH));
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, currentSrc);
        m_BloomDownsampleShader->setInt("srcTexture", 0);
        m_BloomDownsampleShader->setFloat("threshold", m_BloomThreshold);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D, mip.texture->Get(), 0);
        
        dc.SetViewport(0, 0, mip.width, mip.height);
        dc.Clear(BufferBit::Color);
        dc.DrawArrays(Primitive::Triangles, 0, 6);

        currentSrc = mip.texture->Get();
        currentW = mip.width;
        currentH = mip.height;
    }

    m_BloomUpsampleShader->use();
    m_BloomUpsampleShader->setFloat("filterRadius", m_BloomRadius);

    auto& rsm = m_Context->GetRenderStateManager();
    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::One, BlendFactor::One);

    for (size_t i = m_BloomMips.size() - 1; i > 0; --i) {
        const auto& nextMip = m_BloomMips[i];
        const auto& currMip = m_BloomMips[i - 1];

        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, nextMip.texture->Get());
        m_BloomUpsampleShader->setInt("srcTexture", 0);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D, currMip.texture->Get(), 0);

        dc.SetViewport(0, 0, currMip.width, currMip.height);
        dc.DrawArrays(Primitive::Triangles, 0, 6);
    }

    rsm.Disable(ServerCapability::Blend);
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D, m_PingPong.color[1]->Get(), 0);
    dc.SetViewport(0, 0, m_Width, m_Height);
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

    rsm.Disable(ServerCapability::DepthTest);

    shader->use();
    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, m_PingPong.color[0]->Get());
    shader->setInt("screenTexture", 0);

    if (mode == AntiAliasingMode::TAA)
    {
        shader->setInt("depthTexture", 1);
        tm.ActiveTexture(TextureUnit::Texture1);
        tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());

        shader->setInt("historyTexture", 2);
        tm.ActiveTexture(TextureUnit::Texture2);
        tm.BindTexture(TextureType::Texture2D, m_HistoryTexture->Get());

        shader->setMat4("invViewProj", glm::inverse(currViewProj));
        shader->setMat4("prevViewProj", prevViewProj);
        shader->setVec2("jitterOffset", jitterOffset);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        dc.Clear(BufferBit::Color);

        bm.BindVertexArray(m_QuadVAO.id);
        dc.DrawArrays(Primitive::Triangles, 0, 6);

        rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[1]->Get());
        rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_HistoryFBO->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color, TextureFilter::Nearest);

        rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[1]->Get());
        rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_PingPong.fbo[0]->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color, TextureFilter::Nearest);
    }
    else if (mode == AntiAliasingMode::FXAA)
    {
        shader->setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        dc.Clear(BufferBit::Color);

        bm.BindVertexArray(m_QuadVAO.id);
        dc.DrawArrays(Primitive::Triangles, 0, 6);

        rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.fbo[1]->Get());
        rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_PingPong.fbo[0]->Get());
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color, TextureFilter::Nearest);
    }

    bm.BindVertexArray(0);
    rsm.Enable(ServerCapability::DepthTest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader)
{
    AddEffect(shader, 1); // Default priority 1 (Post-Bloom)
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader, int priority)
{
    if (shader)
    {
        m_Effects.push_back({shader, 0, 0, 0, 0, priority});
    }
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader, int x, int y, int w, int h, int priority)
{
    if (shader)
    {
        m_Effects.push_back({shader, x, y, w, h, priority});
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
    bm.BindBuffer(BufferType::ArrayBuffer, m_QuadVBO.id);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), &quadVertices, BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 2, DataType::Float, false, 4 * sizeof(float), (void *)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 2, DataType::Float, false, 4 * sizeof(float), (void *)(2 * sizeof(float)));
}

float PostProcessPipeline::GetGamma() const {
    return m_Gamma;
}

float PostProcessPipeline::GetExposure() const {
    return m_Exposure;
}
