#include <render/unit/gbuffer.h>
#include <core/logic/logger.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <algorithm>

GBuffer::GBuffer()
{
}

GBuffer::~GBuffer()
{
    Shutdown();
}

void GBuffer::Initialize(IGraphicsContext& context, int width, int height)
{
    m_Context = &context;
    m_Width = (std::max)(1, width);
    m_Height = (std::max)(1, height);

    auto& rtm = m_Context->GetRenderTargetManager();
    m_FBO = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());

    CreateTextures();
}

void GBuffer::Shutdown()
{
    m_NormalTexture.reset();
    m_AlbedoSpecTexture.reset();
    m_IDTexture.reset();
    m_EmissiveTexture.reset();
    m_PBRParamsTexture.reset();
    m_DepthTexture.reset();
    m_MultisampleColorTextures.clear();
    m_MultisampleDepthTexture.reset();
    m_MultisampleFBO.reset();
    m_FBO.reset();
}

void GBuffer::Resize(int width, int height)
{
    width = (std::max)(1, width);
    height = (std::max)(1, height);
    if (m_Width == width && m_Height == height && m_AllocatedRenderScale == m_RenderScale &&
        m_AllocatedSampleCount == m_SampleCount && m_AllocatedEntityIdEnabled == m_EntityIdEnabled)
    {
        return;
    }
    m_Width = width;
    m_Height = height;

    // If textures haven't been created yet, do a full creation
    if (!m_NormalTexture)
    {
        CreateTextures();
        return;
    }

    // Reuse existing GPU handles — just re-upload dimensions
    if (!m_Context)
        return;
    auto& tm = m_Context->GetTextureManager();
    auto& rtm = m_Context->GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO->Get());
    int sw = (int)(m_Width * m_RenderScale);
    int sh = (int)(m_Height * m_RenderScale);

    auto resizeTex2D = [&](GPUTexture& tex, InternalFormat fmt, TextureFormat tfmt, DataType dtype) {
        tm.BindTexture(TextureType::Texture2D, tex.Get());
        tm.TexImage2D(TextureType::Texture2D, 0, fmt, sw, sh, 0, tfmt, dtype, nullptr);
    };

    resizeTex2D(*m_NormalTexture, InternalFormat::RGBA8Snorm, TextureFormat::RGBA, DataType::Byte);
    resizeTex2D(*m_AlbedoSpecTexture, InternalFormat::RGBA8, TextureFormat::RGBA, DataType::UnsignedByte);
    if (m_EntityIdEnabled)
    {
        if (!m_IDTexture)
            m_IDTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
        resizeTex2D(*m_IDTexture, InternalFormat::R32UI, TextureFormat::Red_Integer, DataType::UnsignedInt);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                         static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                         static_cast<int>(TextureFilter::Nearest));
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color3,
                                 TextureType::Texture2D, m_IDTexture->Get(), 0);
    }
    else
    {
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color3,
                                 TextureType::Texture2D, 0, 0);
        m_IDTexture.reset();
    }
    resizeTex2D(*m_EmissiveTexture, InternalFormat::RGBA16F, TextureFormat::RGBA, DataType::Float);
    resizeTex2D(*m_PBRParamsTexture, InternalFormat::RGBA8, TextureFormat::RGBA, DataType::UnsignedByte);
    resizeTex2D(*m_DepthTexture, InternalFormat::DepthComponent24, TextureFormat::DepthComponent, DataType::Float);
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    CreateMultisampleTargets();
    m_AllocatedRenderScale = m_RenderScale;
    m_AllocatedSampleCount = m_SampleCount;
    m_AllocatedEntityIdEnabled = m_EntityIdEnabled;
}

void GBuffer::BindForWriting()
{
    if (!m_Context || !m_FBO)
        return;
    auto& rtm = m_Context->GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer,
                        m_SampleCount > 1 && m_MultisampleFBO ? m_MultisampleFBO->Get() : m_FBO->Get());

    FramebufferAttachment attachments[] = {
        FramebufferAttachment::None, FramebufferAttachment::Color1, FramebufferAttachment::Color2,
        m_EntityIdEnabled ? FramebufferAttachment::Color3 : FramebufferAttachment::None,
        FramebufferAttachment::Color4, FramebufferAttachment::Color5};
    rtm.DrawBuffers(6, attachments);
}

void GBuffer::Unbind()
{
    if (!m_Context)
        return;
    if (m_SampleCount > 1)
        Resolve();
    auto& rtm = m_Context->GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    FramebufferAttachment att = FramebufferAttachment::Color0;
    rtm.DrawBuffers(1, &att);
}

void GBuffer::Resolve()
{
    if (!m_Context || !m_FBO || !m_MultisampleFBO || m_SampleCount <= 1)
        return;

    auto& rtm = m_Context->GetRenderTargetManager();
    const int width = GetScaledWidth();
    const int height = GetScaledHeight();
    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_MultisampleFBO->Get());
    rtm.BindFramebuffer(FramebufferTarget::DrawFramebuffer, m_FBO->Get());
    for (int attachmentIndex = 1; attachmentIndex < 6; ++attachmentIndex)
    {
        if (attachmentIndex == 3 && !m_EntityIdEnabled)
            continue;
        const auto attachment = static_cast<FramebufferAttachment>(
            static_cast<int>(FramebufferAttachment::Color0) + attachmentIndex);
        rtm.ReadBuffer(attachment);
        rtm.DrawBuffer(attachment);
        rtm.BlitFramebuffer(0, 0, width, height, 0, 0, width, height, BufferBit::Color, TextureFilter::Nearest);
    }
    rtm.BlitFramebuffer(0, 0, width, height, 0, 0, width, height, BufferBit::Depth, TextureFilter::Nearest);

    FramebufferAttachment attachments[] = {
        FramebufferAttachment::None, FramebufferAttachment::Color1, FramebufferAttachment::Color2,
        m_EntityIdEnabled ? FramebufferAttachment::Color3 : FramebufferAttachment::None,
        FramebufferAttachment::Color4, FramebufferAttachment::Color5};
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO->Get());
    rtm.DrawBuffers(6, attachments);
}

void GBuffer::CreateTextures()
{
    if (!m_Context || !m_FBO)
        return;

    auto& tm = m_Context->GetTextureManager();
    auto& rtm = m_Context->GetRenderTargetManager();

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO->Get());

    m_NormalTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_NormalTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8Snorm, (int)(m_Width * m_RenderScale),
                  (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::Byte, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color1, TextureType::Texture2D,
                             m_NormalTexture->Get(), 0);

    m_AlbedoSpecTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_AlbedoSpecTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, (int)(m_Width * m_RenderScale),
                  (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::UnsignedByte, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color2, TextureType::Texture2D,
                             m_AlbedoSpecTexture->Get(), 0);

    if (m_EntityIdEnabled)
    {
        m_IDTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
        tm.BindTexture(TextureType::Texture2D, m_IDTexture->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::R32UI, (int)(m_Width * m_RenderScale),
                      (int)(m_Height * m_RenderScale), 0, TextureFormat::Red_Integer, DataType::UnsignedInt, nullptr);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                         static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                         static_cast<int>(TextureFilter::Nearest));
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color3,
                                 TextureType::Texture2D, m_IDTexture->Get(), 0);
    }

    m_EmissiveTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_EmissiveTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, (int)(m_Width * m_RenderScale),
                  (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::Float, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color4, TextureType::Texture2D,
                             m_EmissiveTexture->Get(), 0);

    // PBR Params: R: Metallic, G: Roughness, B: Reflectivity, A: Fresnel
    m_PBRParamsTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_PBRParamsTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, (int)(m_Width * m_RenderScale),
                  (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::UnsignedByte, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color5, TextureType::Texture2D,
                             m_PBRParamsTexture->Get(), 0);

    FramebufferAttachment attachments[] = {
        FramebufferAttachment::None, FramebufferAttachment::Color1, FramebufferAttachment::Color2,
        m_EntityIdEnabled ? FramebufferAttachment::Color3 : FramebufferAttachment::None,
        FramebufferAttachment::Color4, FramebufferAttachment::Color5};
    rtm.DrawBuffers(6, attachments);

    m_DepthTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, (int)(m_Width * m_RenderScale),
                  (int)(m_Height * m_RenderScale), 0, TextureFormat::DepthComponent, DataType::Float, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D,
                             m_DepthTexture->Get(), 0);

    if (rtm.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
    {
        LOGGER_ERROR("GBuffer") << "Framebuffer not complete!";
    }

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    m_AllocatedRenderScale = m_RenderScale;
    m_AllocatedEntityIdEnabled = m_EntityIdEnabled;
    CreateMultisampleTargets();
    m_AllocatedSampleCount = m_SampleCount;
}

void GBuffer::CreateMultisampleTargets()
{
    m_MultisampleColorTextures.clear();
    m_MultisampleDepthTexture.reset();
    m_MultisampleFBO.reset();
    if (!m_Context || m_SampleCount <= 1)
        return;

    auto& textures = m_Context->GetTextureManager();
    auto& targets = m_Context->GetRenderTargetManager();
    const int width = GetScaledWidth();
    const int height = GetScaledHeight();
    const InternalFormat formats[] = {InternalFormat::RGBA16F, InternalFormat::RGBA8Snorm, InternalFormat::RGBA8,
                                      InternalFormat::R32UI, InternalFormat::RGBA16F, InternalFormat::RGBA8};

    m_MultisampleFBO = std::make_unique<GPUFramebuffer>(*m_Context, targets.GenFramebuffer());
    targets.BindFramebuffer(FramebufferTarget::Framebuffer, m_MultisampleFBO->Get());
    m_MultisampleColorTextures.reserve(5);
    for (int index = 1; index < 6; ++index)
    {
        if (index == 3 && !m_EntityIdEnabled)
            continue;
        auto texture = std::make_unique<GPUTexture>(*m_Context, textures.GenTexture());
        textures.BindTexture(TextureType::Texture2DMultisample, texture->Get());
        if (!textures.TexImage2DMultisample(TextureType::Texture2DMultisample, m_SampleCount, formats[index], width,
                                            height, true))
        {
            LOGGER_WARN("GBuffer") << "Backend does not support multisampled textures; disabling G-buffer MSAA.";
            m_SampleCount = 1;
            m_MultisampleColorTextures.clear();
            m_MultisampleFBO.reset();
            targets.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
            return;
        }
        const auto attachment =
            static_cast<FramebufferAttachment>(static_cast<int>(FramebufferAttachment::Color0) + index);
        targets.FramebufferTexture2D(FramebufferTarget::Framebuffer, attachment, TextureType::Texture2DMultisample,
                                     texture->Get(), 0);
        m_MultisampleColorTextures.push_back(std::move(texture));
    }

    m_MultisampleDepthTexture = std::make_unique<GPUTexture>(*m_Context, textures.GenTexture());
    textures.BindTexture(TextureType::Texture2DMultisample, m_MultisampleDepthTexture->Get());
    textures.TexImage2DMultisample(TextureType::Texture2DMultisample, m_SampleCount,
                                   InternalFormat::DepthComponent24, width, height, true);
    targets.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth,
                                 TextureType::Texture2DMultisample, m_MultisampleDepthTexture->Get(), 0);

    FramebufferAttachment attachments[] = {
        FramebufferAttachment::None, FramebufferAttachment::Color1, FramebufferAttachment::Color2,
        m_EntityIdEnabled ? FramebufferAttachment::Color3 : FramebufferAttachment::None,
        FramebufferAttachment::Color4, FramebufferAttachment::Color5};
    targets.DrawBuffers(6, attachments);
    if (targets.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
    {
        LOGGER_ERROR("GBuffer") << "Multisampled framebuffer is incomplete; disabling G-buffer MSAA.";
        m_SampleCount = 1;
        m_MultisampleColorTextures.clear();
        m_MultisampleDepthTexture.reset();
        m_MultisampleFBO.reset();
    }
    targets.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

bool GBuffer::ReadEntityId(int x, int y, uint32_t& entityId) const
{
    if (!m_Context || !m_EntityIdEnabled || !m_IDTexture || !m_FBO)
        return false;

    const int width = GetScaledWidth();
    const int height = GetScaledHeight();
    if (x < 0 || y < 0 || x >= width || y >= height)
        return false;

    auto& targets = m_Context->GetRenderTargetManager();
    targets.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_FBO->Get());
    targets.ReadBuffer(FramebufferAttachment::Color3);
    targets.ReadPixels(x, y, 1, 1, TextureFormat::Red_Integer, DataType::UnsignedInt, &entityId);
    targets.BindFramebuffer(FramebufferTarget::ReadFramebuffer, 0);
    return true;
}

bool GBuffer::ReadEntityIds(int x, int y, int width, int height, std::vector<uint32_t>& entityIds) const
{
    if (!m_Context || !m_EntityIdEnabled || !m_IDTexture || !m_FBO || width <= 0 || height <= 0)
        return false;
    const int bufferWidth = GetScaledWidth();
    const int bufferHeight = GetScaledHeight();
    if (x < 0 || y < 0 || x + width > bufferWidth || y + height > bufferHeight)
        return false;

    entityIds.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
    auto& targets = m_Context->GetRenderTargetManager();
    targets.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_FBO->Get());
    targets.ReadBuffer(FramebufferAttachment::Color3);
    targets.ReadPixels(x, y, width, height, TextureFormat::Red_Integer, DataType::UnsignedInt, entityIds.data());
    targets.BindFramebuffer(FramebufferTarget::ReadFramebuffer, 0);
    return true;
}
