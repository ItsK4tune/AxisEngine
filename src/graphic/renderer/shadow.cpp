#include <graphic/renderer/shadow.h>
#include <iostream>
#include <interface/graphic/i_render_target_manager.h>
#include <interface/graphic/i_texture_manager.h>
#include <interface/graphic/i_draw_context.h>

IRenderTargetManager* Shadow::s_RenderTargetManager = nullptr;
ITextureManager* Shadow::s_TextureManager = nullptr;
IDrawContext* Shadow::s_DrawContext = nullptr;

void Shadow::SetManagers(IRenderTargetManager* rtm, ITextureManager* tm, IDrawContext* dc)
{
    s_RenderTargetManager = rtm;
    s_TextureManager = tm;
    s_DrawContext = dc;
}

Shadow::Shadow()
{
    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Dir[i] = 0;
        m_ShadowMap_Dir[i] = 0;
    }
    
    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Point[i] = 0;
        m_ShadowMap_Point[i] = 0;
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Spot[i] = 0;
        m_ShadowMap_Spot[i] = 0;
    }
}

Shadow::~Shadow()
{
    Shutdown();
}

void Shadow::Shutdown()
{
    auto& rtm = GetRenderTargetManager();
    auto& tm = GetTextureManager();

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        if (m_ShadowFBO_Dir[i] != 0)
        {
            rtm.DeleteFramebuffers(1, &m_ShadowFBO_Dir[i]);
            m_ShadowFBO_Dir[i] = 0;
        }
        if (m_ShadowMap_Dir[i] != 0)
        {
            tm.DeleteTextures(1, &m_ShadowMap_Dir[i]);
            m_ShadowMap_Dir[i] = 0;
        }
    }

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        if (m_ShadowFBO_Point[i] != 0)
        {
            rtm.DeleteFramebuffers(1, &m_ShadowFBO_Point[i]);
            m_ShadowFBO_Point[i] = 0;
        }
        if (m_ShadowMap_Point[i] != 0)
        {
            tm.DeleteTextures(1, &m_ShadowMap_Point[i]);
            m_ShadowMap_Point[i] = 0;
        }
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        if (m_ShadowFBO_Spot[i] != 0)
        {
            rtm.DeleteFramebuffers(1, &m_ShadowFBO_Spot[i]);
            m_ShadowFBO_Spot[i] = 0;
        }
        if (m_ShadowMap_Spot[i] != 0)
        {
            tm.DeleteTextures(1, &m_ShadowMap_Spot[i]);
            m_ShadowMap_Spot[i] = 0;
        }
    }
}

void Shadow::Init(unsigned int width, unsigned int height, unsigned int pointWidth, unsigned int pointHeight)
{
    SHADOW_WIDTH = width;
    SHADOW_HEIGHT = height;
    SHADOW_POINT_WIDTH = pointWidth;
    SHADOW_POINT_HEIGHT = pointHeight;

    auto& rtm = GetRenderTargetManager();
    auto& tm = GetTextureManager();

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Dir[i] = rtm.GenFramebuffer();
        m_ShadowMap_Dir[i] = tm.GenTexture();
        
        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Dir[i]);
        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        tm.TexParameterfv(Graphics::TextureType::Texture2D, Graphics::TextureParameter::BorderColor, borderColor);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[i]);
        rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, Graphics::TextureType::Texture2D, m_ShadowMap_Dir[i], 0);
        rtm.DrawBuffer(Graphics::FramebufferAttachment::None);
        rtm.ReadBuffer(Graphics::FramebufferAttachment::None);
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
    }

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Point[i] = rtm.GenFramebuffer();
        m_ShadowMap_Point[i] = tm.GenTexture();
        
        tm.BindTexture(Graphics::TextureType::TextureCubeMap, m_ShadowMap_Point[i]);
        for (unsigned int j = 0; j < 6; ++j)
        {
            tm.TexImage2D(static_cast<Graphics::TextureType>(static_cast<int>(Graphics::TextureType::CubeMapPositiveX) + j), 0, Graphics::InternalFormat::DepthComponent24,
                         SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);
        }
        tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
        tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
        tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::WrapR, static_cast<int>(Graphics::TextureWrap::ClampToEdge));

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Point[i]);
        rtm.FramebufferTexture(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, m_ShadowMap_Point[i], 0);
        rtm.DrawBuffer(Graphics::FramebufferAttachment::None);
        rtm.ReadBuffer(Graphics::FramebufferAttachment::None);
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Spot[i] = rtm.GenFramebuffer();
        m_ShadowMap_Spot[i] = tm.GenTexture();
        
        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Spot[i]);
        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        tm.TexParameterfv(Graphics::TextureType::Texture2D, Graphics::TextureParameter::BorderColor, borderColor);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[i]);
        rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, Graphics::TextureType::Texture2D, m_ShadowMap_Spot[i], 0);
        rtm.DrawBuffer(Graphics::FramebufferAttachment::None);
        rtm.ReadBuffer(Graphics::FramebufferAttachment::None);
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
    }
}

void Shadow::BindFBO_Dir(int index)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[index]);
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::BindFBO_Point(int index)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Point[index]);
        GetDrawContext().SetViewport(0, 0, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT);
    }
}

void Shadow::BindFBO_Spot(int index)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[index]);
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::UnbindFBO()
{
    GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
}

void Shadow::BindTexture_Dir(int index, int unit)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<Graphics::TextureUnit>((int)Graphics::TextureUnit::Texture0 + unit));
        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Dir[index]);
    }
}

void Shadow::BindTexture_Point(int index, int unit)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<Graphics::TextureUnit>((int)Graphics::TextureUnit::Texture0 + unit));
        tm.BindTexture(Graphics::TextureType::TextureCubeMap, m_ShadowMap_Point[index]);
    }
}

void Shadow::BindTexture_Spot(int index, int unit)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<Graphics::TextureUnit>((int)Graphics::TextureUnit::Texture0 + unit));
        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Spot[index]);
    }
}
