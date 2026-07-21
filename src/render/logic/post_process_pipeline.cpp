#include <render/logic/post_process_pipeline.h>
#include <audio/interface/i_audio_capture_service.h>
#include <audio/logic/audio_service.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_profiler.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <ecs/interface/i_geometry_service.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/logic/render_system.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/transient_buffer_ring.h>
#include <render/type/shader_abi.h>
#include <resource/logic/resource_manager.h>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{
static_assert(static_cast<size_t>(ShaderABI::MaxAudioPulses) == AudioPulseLimits::MaxPulses,
              "Audio and shader pulse limits must stay synchronized");

using ProfileClock = std::chrono::steady_clock;

float ElapsedMs(ProfileClock::time_point start, ProfileClock::time_point end)
{
    return std::chrono::duration<float, std::milli>(end - start).count();
}

bool IsUsableTemporalMatrix(const glm::mat4& matrix)
{
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!std::isfinite(matrix[col][row]))
                return false;
        }
    }
    return std::abs(glm::determinant(matrix)) > 0.000001f;
}
}  // namespace

PostProcessPipeline::PostProcessPipeline()
{
}

PostProcessPipeline::~PostProcessPipeline()
{
    Shutdown();
}

void PostProcessPipeline::Initialize(IGraphicsContext& context, int width, int height, IShaderLibrary& shaderLib)
{
    m_Context = &context;
    m_Width = (std::max)(1, width);
    m_Height = (std::max)(1, height);
    m_EventSubscriptions.Clear();

    InitQuad();
    InitFramebuffers();

    m_FXAAShader = shaderLib.GetShader("fxaa");
    m_TAAShader = shaderLib.GetShader("taa");
    m_BloomDownsampleShader = shaderLib.GetShader("bloom_down");
    m_BloomUpsampleShader = shaderLib.GetShader("bloom_up");
    m_HDRFinalShader = shaderLib.GetShader("hdr_final");

    UpdateConfig();

    m_EventSubscriptions.Add(
        EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
            const auto& cfg = e.config;
            m_ClearColor = glm::vec4(cfg.render.clearColor[0], cfg.render.clearColor[1], cfg.render.clearColor[2],
                                     cfg.render.clearColor[3]);
            m_BloomEnabled = cfg.render.bloomEnabled;
            m_BloomThreshold = cfg.render.bloomThreshold;
            m_BloomIntensity = cfg.render.bloomIntensity;
            m_BloomRadius = cfg.render.bloomRadius;
            m_HDREnabled = cfg.render.hdrEnabled;
            m_Exposure = cfg.render.hdrEnabled ? cfg.render.exposure : 1.0f;
            m_Gamma = cfg.render.hdrEnabled ? cfg.render.gamma : 2.2f;
            m_TonemappingMode = cfg.render.hdrEnabled ? (int)cfg.render.tonemappingMode : 0;
        }));
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
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, m_Width, m_Height, 0,
                  TextureFormat::DepthComponent, DataType::Float, NULL);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));

    for (unsigned int i = 0; i < 2; i++)
    {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[i]->Get());
        tm.BindTexture(TextureType::Texture2D, m_PingPong.color[i]->Get());

        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, m_Width, m_Height, 0, TextureFormat::RGBA,
                      DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));

        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D,
                                 m_PingPong.color[i]->Get(), 0);

        if (i == 0)
        {
            rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth,
                                     TextureType::Texture2D, m_DepthTexture->Get(), 0);
        }

        if (rtm.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
            LOGGER_ERROR("PostProcess") << "FBO " << i << " is not complete!";
    }

    for (int i = 0; i < 2; ++i)
    {
        m_HistoryFBO[i] = std::make_unique<GPUFramebuffer>(*m_Context, rtm.GenFramebuffer());
        m_HistoryTexture[i] = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_HistoryFBO[i]->Get());
        tm.BindTexture(TextureType::Texture2D, m_HistoryTexture[i]->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, m_Width, m_Height, 0,
                      TextureFormat::RGBA, DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                         static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                         static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS,
                         static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT,
                         static_cast<int>(TextureWrap::ClampToEdge));
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0,
                                 TextureType::Texture2D, m_HistoryTexture[i]->Get(), 0);
        if (rtm.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
            LOGGER_ERROR("PostProcess") << "History FBO " << i << " is not complete!";
    }
    m_HistoryIndex = 0;

    m_BloomMips.clear();
    int bloomW = std::max(1, m_Width / 2);
    int bloomH = std::max(1, m_Height / 2);
    for (int i = 0; i < BLOOM_MIP_COUNT; i++)
    {
        BloomMip mip;
        mip.width = bloomW;
        mip.height = bloomH;
        mip.texture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
        tm.BindTexture(TextureType::Texture2D, mip.texture->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, bloomW, bloomH, 0, TextureFormat::RGBA,
                      DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));

        m_BloomMips.push_back(std::move(mip));
        bloomW = std::max(1, bloomW / 2);
        bloomH = std::max(1, bloomH / 2);
    }

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::UpdateConfig()
{
    const auto cfg = ServiceLocator::Instance().Require<ConfigManager>().GetConfigSnapshot();
    m_ClearColor = glm::vec4(cfg->render.clearColor[0], cfg->render.clearColor[1], cfg->render.clearColor[2],
                             cfg->render.clearColor[3]);
    m_BloomEnabled = cfg->render.bloomEnabled;
    m_BloomThreshold = cfg->render.bloomThreshold;
    m_BloomIntensity = cfg->render.bloomIntensity;
    m_BloomRadius = cfg->render.bloomRadius;
    m_HDREnabled = cfg->render.hdrEnabled;
    m_Exposure = cfg->render.hdrEnabled ? cfg->render.exposure : 1.0f;
    m_Gamma = cfg->render.hdrEnabled ? cfg->render.gamma : 2.2f;
    m_TonemappingMode = cfg->render.hdrEnabled ? (int)cfg->render.tonemappingMode : 0;
}

void PostProcessPipeline::Shutdown()
{
    m_EventSubscriptions.Clear();
    if (!m_Context)
        return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();

    m_PingPong.fbo[0].reset();
    m_PingPong.fbo[1].reset();
    m_PingPong.color[0].reset();
    m_PingPong.color[1].reset();
    m_DepthTexture.reset();
    CommitPulseUpload();
    m_PulseUpload.reset();
    m_PulseBufferCapacity = 0;
    m_HistoryFBO[0].reset();
    m_HistoryFBO[1].reset();
    m_HistoryTexture[0].reset();
    m_HistoryTexture[1].reset();
    m_BloomMips.clear();
    if (m_QuadVAO.IsValid())
    {
        bm.DeleteVertexArrays(1, &m_QuadVAO.id);
        m_QuadVAO.Reset();
    }
    if (m_QuadVBO.IsValid())
    {
        bm.DeleteBuffers(1, &m_QuadVBO.id);
        m_QuadVBO.Reset();
    }
    m_Context = nullptr;
}

void PostProcessPipeline::Resize(int width, int height)
{
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (m_Width == width && m_Height == height)
        return;
    m_Width = width;
    m_Height = height;
    m_PingPong.ResetToCapture();
    m_HistoryIndex = 0;
    m_ResetTemporalHistory = true;
    if (!m_Context)
        return;
    auto& tm = m_Context->GetTextureManager();

    for (unsigned int i = 0; i < 2; i++)
    {
        tm.BindTexture(TextureType::Texture2D, m_PingPong.color[i]->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, width, height, 0, TextureFormat::RGBA,
                      DataType::Float, NULL);
    }

    tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, width, height, 0,
                  TextureFormat::DepthComponent, DataType::Float, NULL);

    for (auto& historyTexture : m_HistoryTexture)
    {
        tm.BindTexture(TextureType::Texture2D, historyTexture->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, width, height, 0, TextureFormat::RGBA,
                      DataType::Float, NULL);
    }

    int bloomW = std::max(1, width / 2);
    int bloomH = std::max(1, height / 2);
    for (auto& mip : m_BloomMips)
    {
        mip.width = bloomW;
        mip.height = bloomH;
        tm.BindTexture(TextureType::Texture2D, mip.texture->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, bloomW, bloomH, 0, TextureFormat::RGBA,
                      DataType::Float, NULL);
        bloomW = std::max(1, bloomW / 2);
        bloomH = std::max(1, bloomH / 2);
    }
}

void PostProcessPipeline::BeginCapture()
{
    if (!m_Context)
        return;
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& dc = m_Context->GetDrawContext();

    m_PingPong.ResetToCapture();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[0]->Get());
    rsm.SetViewport(0, 0, m_Width, m_Height);
    rsm.Enable(ServerCapability::DepthTest);
    dc.ClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    dc.Clear(BufferBit::Color | BufferBit::Depth);
}

void PostProcessPipeline::EndCapture()
{
    if (!m_Context)
        return;
    const auto priorityOrder = [](const auto& left, const auto& right) {
        return left.priority < right.priority;
    };
    if (m_EffectsDirty)
    {
        std::stable_sort(m_Effects.begin(), m_Effects.end(), priorityOrder);
        m_EffectsDirty = false;
    }
    PrepareFrameInputs();
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    rsm.Disable(ServerCapability::DepthTest);

    // 1. Pre-Bloom Effects (Priority < 0)
    RenderEffectsRange(-9999, -1, false);

    // 2. Engine Bloom
    const bool canRenderBloom = m_BloomEnabled && m_BloomDownsampleShader && m_BloomUpsampleShader &&
                                !m_BloomMips.empty();
    if (canRenderBloom)
    {
        RenderBloom(m_PingPong.CurrentColor().Get());
    }

    // 3. Post-Bloom / Pre-HDR Effects (Priority 0 - 99)
    RenderEffectsRange(0, 99, false);

    // 4. Engine HDR / Tonemapping
    bool hasLateEffects = false;
    for (const auto& eff : m_Effects)
    {
        if (eff.priority >= 100)
        {
            hasLateEffects = true;
            break;
        }
    }

    if (hasLateEffects || HasUIEffects())
    {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.PreviousFBO().Get());
    }
    else
    {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
        rsm.SetViewport(0, 0, m_Width, m_Height);
    }

    dc.Clear(BufferBit::Color);

    if (!m_HDRFinalShader)
    {
        LOGGER_ERROR("PostProcess") << "Required shader 'hdr_final' is missing; using a direct color blit";
        const uint32_t fallbackTarget =
            (hasLateEffects || HasUIEffects()) ? m_PingPong.PreviousFBO().Get() : 0;
        rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.CurrentFBO().Get());
        rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, fallbackTarget);
        rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color,
                            TextureFilter::Nearest);

        if (hasLateEffects || HasUIEffects())
        {
            m_PingPong.Swap();
            RenderEffectsRange(100, 9999, false);
            if (!HasUIEffects())
            {
                rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.CurrentFBO().Get());
                rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, 0);
                rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color,
                                    TextureFilter::Nearest);
            }
        }
        bm.BindVertexArray(0);
        rsm.Enable(ServerCapability::DepthTest);
        if (!HasUIEffects())
        {
            rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
            CommitPulseUpload();
        }
        return;
    }

    m_HDRFinalShader->use();
    m_HDRFinalShader->setInt("screenTexture", 0);
    m_HDRFinalShader->setInt("bloomBlur", 1);
    m_HDRFinalShader->setFloat("exposure", m_HDREnabled ? m_Exposure : 1.0f);
    m_HDRFinalShader->setFloat("bloomIntensity", canRenderBloom ? m_BloomIntensity : 0.0f);
    m_HDRFinalShader->setFloat("gamma", m_HDREnabled ? m_Gamma : 2.2f);
    m_HDRFinalShader->setInt("tonemappingMode", m_HDREnabled ? m_TonemappingMode : 0);

    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, m_PingPong.CurrentColor().Get());

    tm.ActiveTexture(TextureUnit::Texture1);
    if (canRenderBloom)
    {
        tm.BindTexture(TextureType::Texture2D, m_BloomMips[0].texture->Get());
    }
    else
    {
        tm.BindTexture(TextureType::Texture2D, m_PingPong.PreviousColor().Get());
    }

    bm.BindVertexArray(m_QuadVAO.id);
    dc.DrawArrays(Primitive::Triangles, 0, 6);

    if (hasLateEffects || HasUIEffects())
    {
        m_PingPong.Swap();
        // 5. Post-HDR Effects (Priority >= 100, Pre-UI only)
        RenderEffectsRange(100, 9999, false);

        if (!HasUIEffects())
        {
            // Final blit to screen (if no UI effects to follow)
            rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.CurrentFBO().Get());
            rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, 0);
            rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color,
                                TextureFilter::Nearest);
        }
    }

    bm.BindVertexArray(0);
    rsm.Enable(ServerCapability::DepthTest);

    if (!HasUIEffects())
    {
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
        CommitPulseUpload();
    }
}

void PostProcessPipeline::PrepareFrameInputs()
{
    // Defensive completion for clients that registered UI effects but skipped
    // the UI render stage in the preceding frame.
    CommitPulseUpload();
    m_FrameInputs = {};
    m_PulseBufferOffset = 0;
    m_PulseBufferSize = 0;
    m_FrameInputs.depthTexture = m_DepthTexture ? m_DepthTexture->Get() : 0;

    bool needsDeferredData = false;
    bool needsCamera = false;
    bool needsAudio = false;
    for (const auto& effect : m_Effects)
    {
        needsDeferredData = needsDeferredData || HasPostProcessInput(effect.inputs, PostProcessInput::Normal);
        // World position is deliberately reconstructed from the capture depth.
        // This keeps it available without restoring a full-resolution G-buffer MRT.
        needsCamera = needsCamera || HasPostProcessInput(effect.inputs, PostProcessInput::CameraMatrices) ||
                      HasPostProcessInput(effect.inputs, PostProcessInput::WorldPosition);
        needsAudio = needsAudio || HasPostProcessInput(effect.inputs, PostProcessInput::AudioPulses);
    }

    auto& services = ServiceLocator::Instance();
    if (needsDeferredData)
    {
        if (auto* geometry = services.Resolve<IGeometryService>(); geometry && geometry->IsDeferredRenderingEnabled())
        {
            m_FrameInputs.normalTexture = geometry->GetGBufferNormal();
        }
    }
    if (needsCamera)
    {
        if (auto* renderService = services.Resolve<IRenderService>())
        {
            const glm::mat4 viewProjection = renderService->GetCurrViewProj();
            m_FrameInputs.hasCameraMatrices = IsUsableTemporalMatrix(viewProjection);
            if (m_FrameInputs.hasCameraMatrices)
                m_FrameInputs.inverseViewProjection = glm::inverse(viewProjection);
        }
    }
    if (needsAudio)
    {
        if (auto* audioCapture = services.Resolve<IAudioCaptureService>(); audioCapture && audioCapture->IsCapturing())
            m_FrameInputs.audio = audioCapture->GetSnapshot();

        if (auto* audioService = services.Resolve<AudioService>())
        {
            const auto& gameplayPulses = audioService->GetPulses();
            m_FrameInputs.audio.pulses.insert(m_FrameInputs.audio.pulses.end(), gameplayPulses.begin(),
                                              gameplayPulses.end());
        }

        // Capture and gameplay each retain up to the public limit. When both
        // are active, keep the newest events across the merged GPU buffer.
        if (m_FrameInputs.audio.pulses.size() > AudioPulseLimits::MaxPulses)
        {
            std::stable_sort(m_FrameInputs.audio.pulses.begin(), m_FrameInputs.audio.pulses.end(),
                             [](const AudioPulse& lhs, const AudioPulse& rhs) { return lhs.age < rhs.age; });
            m_FrameInputs.audio.pulses.resize(AudioPulseLimits::MaxPulses);
        }

        m_FrameInputs.pulseCount =
            (std::min)(m_FrameInputs.audio.pulses.size(), static_cast<size_t>(ShaderABI::MaxAudioPulses));
        auto& bm = m_Context->GetBufferManager();
        if (!m_PulseUpload)
        {
            m_PulseUpload = std::make_unique<TransientBufferRing>();
            m_PulseBufferCapacity = static_cast<size_t>(ShaderABI::MaxAudioPulses) * sizeof(AudioPulse);
            m_PulseUpload->Initialize(bm, BufferType::ShaderStorageBuffer, m_PulseBufferCapacity);
        }
        if (m_FrameInputs.pulseCount > 0)
        {
            const auto slice = m_PulseUpload->Upload(m_FrameInputs.audio.pulses.data(),
                                                     m_FrameInputs.pulseCount * sizeof(AudioPulse));
            m_PulseBufferOffset = slice.offset;
            m_PulseBufferSize = m_PulseUpload->GetSegmentCapacity();
            m_PulseUploadPending = slice.buffer != 0;
        }
    }
    m_FrameInputs.prepared = true;
}

void PostProcessPipeline::CommitPulseUpload()
{
    if (m_PulseUpload && m_PulseUploadPending)
        m_PulseUpload->Commit();
    m_PulseUploadPending = false;
}

void PostProcessPipeline::RenderEffectsRange(int minPriority, int maxPriority, bool affectUI)
{
    const auto matchesPass = [=](const PostProcessEffect& effect) {
        return effect.shader && effect.priority >= minPriority && effect.priority <= maxPriority &&
               effect.affectUI == affectUI;
    };
    if (std::none_of(m_Effects.begin(), m_Effects.end(), matchesPass))
        return;

    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    for (const auto& effect : m_Effects)
    {
        if (!effect.shader || effect.priority < minPriority || effect.priority > maxPriority ||
            effect.affectUI != affectUI)
            continue;

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.PreviousFBO().Get());

        // If not full-screen, we must preserve the background
        const int effectWidth = effect.width > 0 ? effect.width : m_Width;
        const int effectHeight = effect.height > 0 ? effect.height : m_Height;
        const int effectX = std::clamp(effect.x, 0, m_Width);
        const int effectY = std::clamp(effect.y, 0, m_Height);
        const int clippedWidth = (std::max)(0, (std::min)(effectWidth, m_Width - effectX));
        const int clippedHeight = (std::max)(0, (std::min)(effectHeight, m_Height - effectY));
        const bool isPartial = effectX != 0 || effectY != 0 || clippedWidth != m_Width || clippedHeight != m_Height;
        if (clippedWidth == 0 || clippedHeight == 0)
            continue;
        if (isPartial)
        {
            rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.CurrentFBO().Get());
            rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_PingPong.PreviousFBO().Get());
            rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color,
                                TextureFilter::Nearest);
            rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.PreviousFBO().Get());
        }

        effect.shader->use();
        const bool wantsColor = HasPostProcessInput(effect.inputs, PostProcessInput::Color);
        const bool wantsDepth = HasPostProcessInput(effect.inputs, PostProcessInput::Depth);
        const bool wantsNormal = HasPostProcessInput(effect.inputs, PostProcessInput::Normal);
        const bool wantsWorldPosition = HasPostProcessInput(effect.inputs, PostProcessInput::WorldPosition);
        const bool wantsCamera = HasPostProcessInput(effect.inputs, PostProcessInput::CameraMatrices);
        const bool wantsPulses = HasPostProcessInput(effect.inputs, PostProcessInput::AudioPulses);
        if (wantsColor)
        {
            effect.shader->setInt("screenTexture", ShaderABI::PostProcessColorTexture);
            effect.shader->setInt("u_ScreenTexture", ShaderABI::PostProcessColorTexture);
        }
        const bool canReconstructWorldPosition =
            wantsWorldPosition && m_FrameInputs.depthTexture != 0 && m_FrameInputs.hasCameraMatrices;
        if (wantsDepth || canReconstructWorldPosition)
            effect.shader->setInt("u_DepthTexture", ShaderABI::PostProcessDepthTexture);
        if (wantsNormal)
            effect.shader->setInt("u_NormalTexture", ShaderABI::PostProcessNormalTexture);
        if (wantsWorldPosition)
            effect.shader->setInt("u_WorldPositionTexture", ShaderABI::PostProcessWorldPositionTexture);
        if (wantsCamera || wantsWorldPosition)
            effect.shader->setMat4("u_InverseViewProjection", m_FrameInputs.inverseViewProjection);
        if (wantsPulses)
        {
            const auto& level = m_FrameInputs.audio.level;
            effect.shader->setVec4("u_AudioLevel", glm::vec4(level.rms, level.peak, level.intensity, level.noiseFloor));
            effect.shader->setInt("u_PulseCount", static_cast<int>(m_FrameInputs.pulseCount));
        }
        effect.shader->setBool("u_HasDepthTexture", wantsDepth && m_FrameInputs.depthTexture != 0);
        effect.shader->setBool("u_HasNormalTexture", wantsNormal && m_FrameInputs.normalTexture != 0);
        effect.shader->setBool("u_HasWorldPositionTexture",
                               wantsWorldPosition && m_FrameInputs.worldPositionTexture != 0);
        effect.shader->setBool("u_HasWorldPosition",
                               wantsWorldPosition &&
                                   (m_FrameInputs.worldPositionTexture != 0 || canReconstructWorldPosition));
        effect.shader->setBool("u_WorldPositionFromDepth", canReconstructWorldPosition);
        effect.shader->setBool("u_HasCameraMatrices",
                               (wantsCamera || wantsWorldPosition) && m_FrameInputs.hasCameraMatrices);
        effect.shader->setBool("u_HasAudioPulses", wantsPulses && m_FrameInputs.pulseCount > 0);
        effect.shader->setBool("u_IsPartialEffect", isPartial);
        effect.shader->setVec4(
            "u_EffectRect",
            glm::vec4(static_cast<float>(effectX) / m_Width, static_cast<float>(effectY) / m_Height,
                      static_cast<float>(clippedWidth) / m_Width,
                      static_cast<float>(clippedHeight) / m_Height));

        tm.ActiveTexture(static_cast<TextureUnit>(ShaderABI::PostProcessColorTexture));
        tm.BindTexture(TextureType::Texture2D, wantsColor ? m_PingPong.CurrentColor().Get() : 0);
        tm.ActiveTexture(static_cast<TextureUnit>(ShaderABI::PostProcessDepthTexture));
        tm.BindTexture(TextureType::Texture2D,
                       (wantsDepth || canReconstructWorldPosition) ? m_FrameInputs.depthTexture : 0);
        tm.ActiveTexture(static_cast<TextureUnit>(ShaderABI::PostProcessNormalTexture));
        tm.BindTexture(TextureType::Texture2D, wantsNormal ? m_FrameInputs.normalTexture : 0);
        tm.ActiveTexture(static_cast<TextureUnit>(ShaderABI::PostProcessWorldPositionTexture));
        tm.BindTexture(TextureType::Texture2D, wantsWorldPosition ? m_FrameInputs.worldPositionTexture : 0);
        if (wantsPulses && m_PulseUpload && m_PulseBufferSize > 0)
            bm.BindBufferRange(BufferType::ShaderStorageBuffer, ShaderABI::PulseSSBOBinding,
                               m_PulseUpload->GetBuffer(), m_PulseBufferOffset, m_PulseBufferSize);

        if (isPartial)
        {
            // A full-screen quad plus scissor preserves screen-space UVs and
            // avoids rebuilding/uploading a unique quad for every partial effect.
            rsm.Enable(ServerCapability::ScissorTest);
            rsm.SetScissor(effectX, m_Height - effectY - clippedHeight, clippedWidth, clippedHeight);
            dc.SetViewport(0, 0, m_Width, m_Height);
            bm.BindVertexArray(m_QuadVAO.id);
            dc.DrawArrays(Primitive::Triangles, 0, 6);
            bm.BindVertexArray(0);
            rsm.Disable(ServerCapability::ScissorTest);
        }
        else
        {
            dc.SetViewport(0, 0, m_Width, m_Height);
            bm.BindVertexArray(m_QuadVAO.id);
            dc.DrawArrays(Primitive::Triangles, 0, 6);
            bm.BindVertexArray(0);
        }

        m_PingPong.Swap();
    }
}

void PostProcessPipeline::RenderBloom(uint32_t srcTexture)
{
    const auto bloomStart = ProfileClock::now();

    auto& rtm = m_Context->GetRenderTargetManager();
    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();

    bm.BindVertexArray(m_QuadVAO.id);

    m_BloomDownsampleShader->use();
    uint32_t currentSrc = srcTexture;
    int currentW = m_Width;
    int currentH = m_Height;

    for (const auto& mip : m_BloomMips)
    {
        m_BloomDownsampleShader->setVec2("srcResolution", glm::vec2(currentW, currentH));
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, currentSrc);
        m_BloomDownsampleShader->setInt("srcTexture", 0);
        m_BloomDownsampleShader->setFloat("threshold", (currentSrc == srcTexture) ? m_BloomThreshold : 0.0f);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D,
                                 mip.texture->Get(), 0);

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

    for (size_t i = m_BloomMips.size() - 1; i > 0; --i)
    {
        const auto& nextMip = m_BloomMips[i];
        const auto& currMip = m_BloomMips[i - 1];

        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, nextMip.texture->Get());
        m_BloomUpsampleShader->setInt("srcTexture", 0);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.fbo[1]->Get());
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D,
                                 currMip.texture->Get(), 0);

        dc.SetViewport(0, 0, currMip.width, currMip.height);
        dc.DrawArrays(Primitive::Triangles, 0, 6);
    }

    rsm.Disable(ServerCapability::Blend);
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D,
                             m_PingPong.color[1]->Get(), 0);
    dc.SetViewport(0, 0, m_Width, m_Height);

    RuntimeProfiler::Instance().SetPassTime(ProfiledRenderPass::Bloom, ElapsedMs(bloomStart, ProfileClock::now()));
}

void PostProcessPipeline::ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4& prevViewProj,
                                            const glm::mat4& currViewProj, const glm::vec2& jitterOffset)
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
    tm.BindTexture(TextureType::Texture2D, m_PingPong.CurrentColor().Get());
    shader->setInt("screenTexture", 0);

    if (mode == AntiAliasingMode::TAA)
    {
        const bool validTemporalMatrices = IsUsableTemporalMatrix(prevViewProj) && IsUsableTemporalMatrix(currViewProj);
        const bool resetHistory = m_ResetTemporalHistory || !validTemporalMatrices;
        const glm::mat4 safeCurrViewProj = IsUsableTemporalMatrix(currViewProj) ? currViewProj : glm::mat4(1.0f);

        shader->setInt("depthTexture", 1);
        tm.ActiveTexture(TextureUnit::Texture1);
        tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());

        shader->setInt("historyTexture", 2);
        tm.ActiveTexture(TextureUnit::Texture2);
        tm.BindTexture(TextureType::Texture2D, m_HistoryTexture[m_HistoryIndex]->Get());

        shader->setBool("resetHistory", resetHistory);
        shader->setMat4("invViewProj", resetHistory ? glm::mat4(1.0f) : glm::inverse(currViewProj));
        shader->setMat4("prevViewProj", resetHistory ? safeCurrViewProj : prevViewProj);
        shader->setVec2("jitterOffset", jitterOffset);

        const int nextHistory = 1 - m_HistoryIndex;
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_HistoryFBO[nextHistory]->Get());
        dc.Clear(BufferBit::Color);

        bm.BindVertexArray(m_QuadVAO.id);
        dc.DrawArrays(Primitive::Triangles, 0, 6);

        m_HistoryIndex = nextHistory;
        m_PingPong.SetExternalCurrent(*m_HistoryFBO[m_HistoryIndex], *m_HistoryTexture[m_HistoryIndex]);
        m_ResetTemporalHistory = false;
    }
    else if (mode == AntiAliasingMode::FXAA)
    {
        shader->setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_PingPong.PreviousFBO().Get());
        dc.Clear(BufferBit::Color);

        bm.BindVertexArray(m_QuadVAO.id);
        dc.DrawArrays(Primitive::Triangles, 0, 6);

        m_PingPong.Swap();
    }

    bm.BindVertexArray(0);
    rsm.Enable(ServerCapability::DepthTest);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader, bool affectUI, PostProcessInput inputs)
{
    AddEffect(shader, 0, 0, 0, 0, 0, affectUI, inputs);
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader, int priority, bool affectUI,
                                    PostProcessInput inputs)
{
    AddEffect(shader, 0, 0, 0, 0, priority, affectUI, inputs);
}

void PostProcessPipeline::AddEffect(std::shared_ptr<Shader> shader, int x, int y, int width, int height, int priority,
                                    bool affectUI, PostProcessInput inputs)
{
    if (shader)
    {
        m_Effects.push_back({shader, x, y, width, height, priority, affectUI, inputs});
        m_EffectsDirty = true;
        m_HasUIEffects = m_HasUIEffects || affectUI;
    }
}

bool PostProcessPipeline::HasUIEffects() const
{
    return m_HasUIEffects;
}

void PostProcessPipeline::RenderUIEffects()
{
    if (!m_Context || !HasUIEffects())
        return;

    auto& rtm = m_Context->GetRenderTargetManager();

    // Render all effects marked as affectUI
    RenderEffectsRange(-9999, 9999, true);

    // Final blit to screen
    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_PingPong.CurrentFBO().Get());
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, 0);
    rtm.BlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, BufferBit::Color, TextureFilter::Nearest);
    CommitPulseUpload();
}

void PostProcessPipeline::ClearEffects()
{
    CommitPulseUpload();
    m_Effects.clear();
    m_EffectsDirty = false;
    m_HasUIEffects = false;
    m_FrameInputs = {};
}

void PostProcessPipeline::ResetTemporalHistory()
{
    m_ResetTemporalHistory = true;
    m_PingPong.ResetToCapture();
    m_HistoryIndex = 0;
}

void PostProcessPipeline::InitQuad()
{
    if (!m_Context)
        return;
    auto& bm = m_Context->GetBufferManager();

    float quadVertices[] = {-1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

                            -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

    m_QuadVAO.id = bm.GenVertexArray();
    m_QuadVBO.id = bm.GenBuffer();

    bm.BindVertexArray(m_QuadVAO.id);
    bm.BindBuffer(BufferType::ArrayBuffer, m_QuadVBO.id);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), &quadVertices, BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 2, DataType::Float, false, 4 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 2, DataType::Float, false, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}
