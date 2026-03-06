#include <graphics/renderer/shadow.h>
#include <graphics/interfaces/i_graphics_context.h>

#include <graphics/interfaces/i_draw_context.h>
#include <graphics/interfaces/i_render_target_manager.h>
#include <graphics/interfaces/i_texture_manager.h>
#include <iostream>

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

}

Shadow::~Shadow()
{
    Shutdown();
}

void Shadow::Shutdown()
{
    auto& rtm = GetRenderTargetManager();
    auto& tm = GetTextureManager();

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i) { m_ShadowFBO_Dir[i].reset(); m_ShadowMap_Dir[i].reset(); }
    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i) { m_ShadowFBO_Point[i].reset(); m_ShadowMap_Point[i].reset(); }
    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i) { m_ShadowFBO_Spot[i].reset(); m_ShadowMap_Spot[i].reset(); }
}

void Shadow::Init(IGraphicsContext& context, unsigned int width, unsigned int height, unsigned int pointWidth, unsigned int pointHeight)
{
    SHADOW_WIDTH = width;
    SHADOW_HEIGHT = height;
    SHADOW_POINT_WIDTH = pointWidth;
    SHADOW_POINT_HEIGHT = pointHeight;

    auto& rtm = GetRenderTargetManager();
    auto& tm = GetTextureManager();

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Dir[i] = std::make_unique<Graphics::GPUFramebuffer>(context, rtm.GenFramebuffer());
        m_ShadowMap_Dir[i] = std::make_unique<Graphics::GPUTexture>(context, tm.GenTexture());

        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Dir[i]->Get());
        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        tm.TexParameterfv(Graphics::TextureType::Texture2D, Graphics::TextureParameter::BorderColor, borderColor);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[i]->Get());
        rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, Graphics::TextureType::Texture2D, m_ShadowMap_Dir[i]->Get(), 0);
        rtm.DrawBuffer(Graphics::FramebufferAttachment::None);
        rtm.ReadBuffer(Graphics::FramebufferAttachment::None);
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
    }

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Point[i] = std::make_unique<Graphics::GPUFramebuffer>(context, rtm.GenFramebuffer());
        m_ShadowMap_Point[i] = std::make_unique<Graphics::GPUTexture>(context, tm.GenTexture());

        tm.BindTexture(Graphics::TextureType::TextureCubeMap, m_ShadowMap_Point[i]->Get());
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

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Point[i]->Get());
        rtm.FramebufferTexture(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, m_ShadowMap_Point[i]->Get(), 0);
        rtm.DrawBuffer(Graphics::FramebufferAttachment::None);
        rtm.ReadBuffer(Graphics::FramebufferAttachment::None);
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Spot[i] = std::make_unique<Graphics::GPUFramebuffer>(context, rtm.GenFramebuffer());
        m_ShadowMap_Spot[i] = std::make_unique<Graphics::GPUTexture>(context, tm.GenTexture());

        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Spot[i]->Get());
        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, Graphics::TextureFormat::DepthComponent, Graphics::DataType::Float, NULL);
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToBorder));
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        tm.TexParameterfv(Graphics::TextureType::Texture2D, Graphics::TextureParameter::BorderColor, borderColor);

        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[i]->Get());
        rtm.FramebufferTexture2D(Graphics::FramebufferTarget::Framebuffer, Graphics::FramebufferAttachment::Depth, Graphics::TextureType::Texture2D, m_ShadowMap_Spot[i]->Get(), 0);
        rtm.DrawBuffer(Graphics::FramebufferAttachment::None);
        rtm.ReadBuffer(Graphics::FramebufferAttachment::None);
        rtm.BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, 0);
    }
}

void Shadow::BindFBO_Dir(int index)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::BindFBO_Point(int index)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Point[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT);
    }
}

void Shadow::BindFBO_Spot(int index)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(Graphics::FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[index]->Get());
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
        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Dir[index]->Get());
    }
}

void Shadow::BindTexture_Point(int index, int unit)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<Graphics::TextureUnit>((int)Graphics::TextureUnit::Texture0 + unit));
        tm.BindTexture(Graphics::TextureType::TextureCubeMap, m_ShadowMap_Point[index]->Get());
    }
}

void Shadow::BindTexture_Spot(int index, int unit)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<Graphics::TextureUnit>((int)Graphics::TextureUnit::Texture0 + unit));
        tm.BindTexture(Graphics::TextureType::Texture2D, m_ShadowMap_Spot[index]->Get());
    }
}
