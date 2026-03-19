#include <render/unit/gbuffer.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <core/logic/logger.h>

GBuffer::GBuffer() {}

GBuffer::~GBuffer()
{
    Shutdown();
}

void GBuffer::Initialize(IGraphicsContext& context, int width, int height)
{
    m_Context = &context;
    m_Width = width;
    m_Height = height;

    auto& rtm = m_Context->GetRenderTargetManager();
    m_FBO = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());

    CreateTextures();
}

void GBuffer::Shutdown()
{
    m_PositionTexture.reset();
    m_NormalTexture.reset();
    m_AlbedoSpecTexture.reset();
    m_IDTexture.reset();
    m_DepthTexture.reset();
    m_FBO.reset();
}

void GBuffer::Resize(int width, int height)
{
    if (m_Width == width && m_Height == height) return;
    m_Width = width;
    m_Height = height;
    CreateTextures();
}

void GBuffer::BindForWriting()
{
    if (!m_Context || !m_FBO) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO->Get());
    
    // Always ensure all 4 buffers are enabled for the geometry pass
    FramebufferAttachment attachments[] = { 
        FramebufferAttachment::Color0, 
        FramebufferAttachment::Color1, 
        FramebufferAttachment::Color2,
        FramebufferAttachment::Color3 
    };
    rtm.DrawBuffers(4, attachments);
}

void GBuffer::BindForReading()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::ReadFramebuffer, m_FBO->Get());
}

void GBuffer::Unbind()
{
    if (!m_Context) return;
    auto& rtm = m_Context->GetRenderTargetManager();
    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    FramebufferAttachment att = FramebufferAttachment::Color0;
    rtm.DrawBuffers(1, &att);
}

void GBuffer::CreateTextures()
{
    if (!m_Context || !m_FBO) return;

    auto& tm = m_Context->GetTextureManager();
    auto& rtm = m_Context->GetRenderTargetManager();

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_FBO->Get());

    // Position color buffer
    m_PositionTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_PositionTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, (int)(m_Width * m_RenderScale), (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::Float, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color0, TextureType::Texture2D, m_PositionTexture->Get(), 0);

    // Normal color buffer
    m_NormalTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_NormalTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA16F, (int)(m_Width * m_RenderScale), (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::Float, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color1, TextureType::Texture2D, m_NormalTexture->Get(), 0);
    
    // Albedo + Specular color buffer
    m_AlbedoSpecTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_AlbedoSpecTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, (int)(m_Width * m_RenderScale), (int)(m_Height * m_RenderScale), 0, TextureFormat::RGBA, DataType::UnsignedByte, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color2, TextureType::Texture2D, m_AlbedoSpecTexture->Get(), 0);

    // Entity ID color buffer
    m_IDTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_IDTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::R32UI, (int)(m_Width * m_RenderScale), (int)(m_Height * m_RenderScale), 0, TextureFormat::Red_Integer, DataType::UnsignedInt, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Color3, TextureType::Texture2D, m_IDTexture->Get(), 0);

    // Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    FramebufferAttachment attachments[] = { 
        FramebufferAttachment::Color0, 
        FramebufferAttachment::Color1, 
        FramebufferAttachment::Color2,
        FramebufferAttachment::Color3 
    };
    rtm.DrawBuffers(4, attachments);

    m_DepthTexture = std::make_unique<GPUTexture>(*m_Context, tm.GenTexture());
    tm.BindTexture(TextureType::Texture2D, m_DepthTexture->Get());
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, (int)(m_Width * m_RenderScale), (int)(m_Height * m_RenderScale), 0, TextureFormat::DepthComponent, DataType::Float, nullptr);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D, m_DepthTexture->Get(), 0);

    if (rtm.CheckFramebufferStatus(FramebufferTarget::Framebuffer) != FramebufferStatus::Complete)
    {
        LOGGER_ERROR("GBuffer") << "Framebuffer not complete!";
    }

    rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}
