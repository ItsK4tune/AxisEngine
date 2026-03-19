#include <render/unit/shadow.h>
#include <render/interface/i_graphics_context.h>

#include <render/interface/i_draw_context.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
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

void Shadow::Initialize(IGraphicsContext& context, unsigned int width, unsigned int height, bool manualControl, unsigned int pointWidth, unsigned int pointHeight)
{
    SHADOW_WIDTH = width;
    SHADOW_HEIGHT = height;
    SHADOW_POINT_WIDTH = pointWidth;
    SHADOW_POINT_HEIGHT = pointHeight;

    auto& rtm = GetRenderTargetManager();
    auto& tm = GetTextureManager();

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Dir[i] = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());
        m_ShadowMap_Dir[i] = std::make_unique<GPUTexture>(context, tm.GenTexture());

        tm.BindTexture(TextureType::Texture2D, m_ShadowMap_Dir[i]->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToBorder));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToBorder));
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        tm.TexParameterfv(TextureType::Texture2D, TextureParameter::BorderColor, borderColor);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[i]->Get());
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D, m_ShadowMap_Dir[i]->Get(), 0);
        rtm.DrawBuffer(FramebufferAttachment::None);
        rtm.ReadBuffer(FramebufferAttachment::None);
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    }

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Point[i] = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());
        m_ShadowMap_Point[i] = std::make_unique<GPUTexture>(context, tm.GenTexture());

        tm.BindTexture(TextureType::TextureCubeMap, m_ShadowMap_Point[i]->Get());
        for (unsigned int j = 0; j < 6; ++j)
        {
            tm.TexImage2D(static_cast<TextureType>(static_cast<int>(TextureType::CubeMapPositiveX) + j), 0, InternalFormat::DepthComponent24,
                         SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
        }
        tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));
        tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapR, static_cast<int>(TextureWrap::ClampToEdge));

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Point[i]->Get());
        rtm.FramebufferTexture(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, m_ShadowMap_Point[i]->Get(), 0);
        rtm.DrawBuffer(FramebufferAttachment::None);
        rtm.ReadBuffer(FramebufferAttachment::None);
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Spot[i] = std::make_unique<GPUFramebuffer>(context, rtm.GenFramebuffer());
        m_ShadowMap_Spot[i] = std::make_unique<GPUTexture>(context, tm.GenTexture());

        tm.BindTexture(TextureType::Texture2D, m_ShadowMap_Spot[i]->Get());
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::DepthComponent24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, TextureFormat::DepthComponent, DataType::Float, NULL);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToBorder));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToBorder));
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        tm.TexParameterfv(TextureType::Texture2D, TextureParameter::BorderColor, borderColor);

        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[i]->Get());
        rtm.FramebufferTexture2D(FramebufferTarget::Framebuffer, FramebufferAttachment::Depth, TextureType::Texture2D, m_ShadowMap_Spot[i]->Get(), 0);
        rtm.DrawBuffer(FramebufferAttachment::None);
        rtm.ReadBuffer(FramebufferAttachment::None);
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    }
}

void Shadow::BindFBO_Dir(int index)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Dir[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::BindFBO_Point(int index)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Point[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT);
    }
}

void Shadow::BindFBO_Spot(int index)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, m_ShadowFBO_Spot[index]->Get());
        GetDrawContext().SetViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::UnbindFBO()
{
    GetRenderTargetManager().BindFramebuffer(FramebufferTarget::Framebuffer, 0);
}

void Shadow::BindTexture_Dir(int index, int unit)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<TextureUnit>((int)TextureUnit::Texture0 + unit));
        tm.BindTexture(TextureType::Texture2D, m_ShadowMap_Dir[index]->Get());
    }
}

void Shadow::BindTexture_Point(int index, int unit)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<TextureUnit>((int)TextureUnit::Texture0 + unit));
        tm.BindTexture(TextureType::TextureCubeMap, m_ShadowMap_Point[index]->Get());
    }
}

void Shadow::BindTexture_Spot(int index, int unit)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        auto& tm = GetTextureManager();
        tm.ActiveTexture(static_cast<TextureUnit>((int)TextureUnit::Texture0 + unit));
        tm.BindTexture(TextureType::Texture2D, m_ShadowMap_Spot[index]->Get());
    }
}
